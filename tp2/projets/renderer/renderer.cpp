#include "imageUtils.h"
#include "m256Point.h"
#include "m256Vector.h"
#include "m256Utils.h"
#include "mat.h"
#include "renderer.h"
#include "xorshift.h"

#include <cmath>
#include <cstring>
#include <immintrin.h>
#include <omp.h>

#ifndef M_PI
    #define M_PI 3.141592653589793
#endif

Color Renderer::AMBIENT_COLOR = Color(0.1f, 0.1f, 0.1f);
Color Renderer::BACKGROUND_COLOR = Color(135.0f / 255.0f, 206.0f / 255.0f, 235.0f / 255.0f);//Sky color

Material init_default_material()
{
    Material mat = Material(Color(1.0f, 0.0f, 0.5f));//Pink color
    mat.specular = Color(0.0f, 0.0f, 0.0f);
    mat.ns = 30;
    mat.reflection = 0.0f;

    return mat;
}

/**
 * @brief Creates a material that is not impacted by diffuse lighting
 * so that even viewed from a grazing angle, it stays well illuminated
 * This is not accurate but is well-suited for an infinite Plane as the
 * "floor" of the scene
 * @return The material
 */
Material init_default_plane_material()
{
    Material mat = Material(Color(0.9));//White color
    mat.diffuse = Color(0.3f);
    mat.ambient_coeff = Color((Color(0.8f) / Renderer::AMBIENT_COLOR).r);

    return mat;
}

Material init_debug_material_1()
{
    Material mat = Material(Color(0.5f, 0.5f, 1.0f));//Blueish color
    mat.specular = Color(1.0f, 1.0f, 1.0f);
    mat.ns = 15;

    return mat;
}

Material Renderer::DEFAULT_MATERIAL = init_default_material();
Material Renderer::DEFAULT_PLANE_MATERIAL = init_default_plane_material();
Material Renderer::DEBUG_MATERIAL_1 = init_debug_material_1();

Material Renderer::get_random_diffuse_pastel_material()
{
    Material mat;
    mat.specular = Color(0.0f, 0.0f, 0.0f);

    float random_hue = std::rand() / (float)RAND_MAX * 360;
    float saturation = 1.0f;
    float lightness = 0.75f;

    float r, g, b;
    ImageUtils::HSLtoRGB(r, g, b, random_hue, saturation, lightness);

    mat.diffuse = Color(r, g, b);

    return mat;
}

Renderer::Renderer(Scene scene, std::vector<Triangle> triangles, RenderSettings render_settings) : _triangles(triangles),
    _render_settings(render_settings), _scene(scene)
{
    if (render_settings.enable_bvh)
        _bvh = BVH(&triangles, render_settings.bvh_max_depth);

    //Accounting for the SSAA scaling
    int render_width, render_height;
    get_render_width_height(render_settings, render_width, render_height);

    init_buffers(render_width, render_height);
    _scene._camera.set_aspect_ratio((float)render_width / render_height);
}

Renderer::Renderer() : Renderer(Scene(), std::vector<Triangle>(), RenderSettings::basic_settings(1280, 720)) {}

QImage* Renderer::get_image()
{
    return &_image;
}

RenderSettings& Renderer::render_settings()
{
    return _render_settings;
}

void Renderer::get_render_width_height(const RenderSettings& settings, int& render_width, int& render_height)
{
    render_width = settings.enable_ssaa ? settings.image_width * settings.ssaa_factor : settings.image_width;
    render_height = settings.enable_ssaa ? settings.image_height * settings.ssaa_factor : settings.image_height;
}

void Renderer::init_buffers(int width, int height)
{
    //Initializing the z-buffer if we're using the rasterization approach or post-processing that needs it
    if (_render_settings.hybrid_rasterization_tracing || _render_settings.enable_ssao)
        _z_buffer = Buffer<float>(width, height, INFINITY);

    if (_render_settings.enable_ssao)
        _normal_buffer = Buffer<Vector>(width, height);

    _image = QImage(width, height, QImage::Format_ARGB32);
    for (int i = 0; i < height; i++)
        for (int j = 0; j < width; j++)
            _image.setPixel(j, i, ImageUtils::gkit_color_to_Qt_ARGB32_uint(Renderer::BACKGROUND_COLOR));
}

void Renderer::set_triangles(const std::vector<Triangle>& triangles)

{
    _triangles = triangles;

    if (_render_settings.enable_bvh)
        _bvh = BVH(&_triangles, _render_settings.bvh_max_depth, _render_settings.bvh_leaf_object_count);
}

void Renderer::add_analytic_shape(const AnalyticShapesTypes& shape) {  _analytic_shapes.push_back(shape); }

Materials& Renderer::get_materials() { return _materials; }

void Renderer::set_materials(Materials materials) { _materials = materials; }

void Renderer::prepare_ssao_buffers()
{
    int render_width, render_height;
    get_render_width_height(_render_settings, render_width, render_height);

    _normal_buffer = Buffer<Vector>(render_width, render_height);
}

void Renderer::destroy_ssao_buffers()
{
    _normal_buffer = Buffer<Vector>();
}

void Renderer::clear_z_buffer()
{
    _z_buffer.fill_values(INFINITY);
}

void Renderer::clear_normal_buffer()
{
    _normal_buffer.fill_values(Vector(0, 0, 0));
}

void Renderer::clear_image()
{
    for (int i = 0; i < _image.height(); i++)
        for (int j = 0; j < _image.width(); j++)
            _image.setPixel(j, i, ImageUtils::gkit_color_to_Qt_ARGB32_uint(Renderer::BACKGROUND_COLOR));
}

void Renderer::clear_geometry()
{
    set_triangles(std::vector<Triangle>());
    _analytic_shapes = std::vector<AnalyticShapesTypes>();
}

void Renderer::change_camera_fov(float fov) { _scene._camera.set_fov(fov); }
void Renderer::change_camera_aspect_ratio(float aspect_ratio) { _scene._camera.set_aspect_ratio(aspect_ratio); }
void Renderer::set_light_position(const Point& position) { _scene._point_light._position = position; }

void Renderer::set_ao_map(const Image& ao_map) { _ao_map = ao_map; }
void Renderer::set_diffuse_map(const Image& diffuse_map) { _diffuse_map = diffuse_map; }
void Renderer::set_skysphere(const Image& skybox) { _skysphere = skybox; }
void Renderer::set_skybox(const Skybox& skybox) { _skybox = skybox; }

void Renderer::clear_ao_map() { _ao_map = Image(); }
void Renderer::clear_diffuse_map() { _diffuse_map = Image(); }

Color Renderer::sample_texture(const Image& texture, float& tex_coord_u, float& tex_coord_v) const
{
    return texture.texture(tex_coord_u, tex_coord_v);
}

void Renderer::reset_previous_transform() { _previous_object_transform = Identity(); }

void Renderer::set_object_transform(const Transform& object_transform)
{
    _previous_object_transform = _previous_object_transform.inverse();
    Transform transform = object_transform(_previous_object_transform);

    for (Triangle& triangle : _triangles)
        triangle = transform(triangle);
    _bvh = BVH(&_triangles, _render_settings.bvh_max_depth, _render_settings.bvh_leaf_object_count);

    _previous_object_transform = object_transform;
}

void Renderer::set_camera_transform(const Transform& camera_transform)
{
    _scene._camera._camera_to_world_mat = camera_transform;
    _scene._camera._world_to_camera_mat = camera_transform.inverse();

    //Transforming the original position of the camera
    _scene._camera._position = camera_transform(Point(0, 0, 0));
}

void Renderer::reconstruct_bvh_new()
{
    _bvh = BVH(&_triangles, _render_settings.bvh_max_depth, _render_settings.bvh_leaf_object_count);
}

void Renderer::destroy_bvh() { _bvh = BVH();/* Empty BVH basically destroying the previous one */ }

void Renderer::change_render_size(int width, int height)
{
    _render_settings.image_width = width;
    _render_settings.image_height = height;

    int render_width, render_height;
    get_render_width_height(_render_settings, render_width, render_height);

    init_buffers(render_width, render_height);

    _scene._camera.set_aspect_ratio((float)render_width / render_height);
}

Color Renderer::compute_diffuse(const Material& hitMaterial, const Vector& normal, const Vector& direction_to_light) const
{
    return hitMaterial.diffuse * Color(std::max(0.0f, dot(normal, direction_to_light)));
}

//TODO faire un thread qui tourne en permanence et qui display l'image

Color Renderer::compute_specular(const Material& hit_material, const Vector& ray_direction, const Vector& normal, const Vector& direction_to_light) const
{
    Vector halfway_vector = normalize(direction_to_light - ray_direction);//Blinn-phong
    float angle = dot(halfway_vector, normal);

    //Specular optimization to avoid computing the exponentiation when not necessary (i.e. when it corresponds to a negligeable visual impact)
    if (angle <= hit_material.specular_threshold)//We're below the visibility threshold so we're not going to notice the specular anyway, returning no specular
        return Color(0, 0, 0);
    else
        return hit_material.specular * Color(std::pow(std::max(0.0f, angle), hit_material.ns));
}

//TODO passer inter_point en argument pour eviter de le recalculer a chaque fois vu qu'on l'uilise deja potentiellment autre part etr donc on l'a deja potentiellement calcule
Color Renderer::compute_reflection(const Ray& ray, const HitInfo& hit_info) const
{
    bool intersection_found = false;
    HitInfo reflection_hit_info;

    //TODO normaliser les normales qu'on met dans HitInfo
    Vector normalized_normal = hit_info.normal_at_intersection;
    Point inter_point = ray._origin + ray._direction * hit_info.t;

    Point reflection_ray_origin = inter_point + normalized_normal * 0.01f;
    Vector reflection_ray_direction = ray._direction - 2 * dot(ray._direction, normalized_normal) * normalized_normal;
    Ray reflection_ray(reflection_ray_origin, reflection_ray_direction);

//    std::cout << ray << std::endl;
//    std::cout << inter_point << std::endl;
//    std::cout << normalized_normal << std::endl;
//    std::cout << reflection_ray << std::endl;
//    std::exit(0);

    Color reflection_color = trace_ray(reflection_ray, reflection_hit_info, intersection_found);

    return reflection_color * Color(_materials.material(hit_info.mat_index).reflection);
}

bool Renderer::is_shadowed(const Point& inter_point, const Vector& normal_at_intersection, const Point& light_position) const
{
    if (_render_settings.compute_shadows)
    {
        Ray ray(inter_point + normal_at_intersection * Renderer::EPSILON, normalize(light_position - inter_point));
        HitInfo hitInfo;

        if (_render_settings.enable_bvh)
        {
            if (_bvh.intersect(ray, hitInfo))
            {
                Point new_inter_point = ray._origin + ray._direction * hitInfo.t;

                //If we found an object that is between the light and the current inter_point: the point is shadowed
                if (length2(Point(inter_point) - Point(new_inter_point)) < length2(Point(inter_point) - Point(light_position)))
                    return true;
            }
        }
        else
        {
            for (const Triangle& triangle : _triangles)
            {
                if (triangle.intersect(ray, hitInfo))
                {
                    Point new_inter_point = ray._origin + ray._direction * hitInfo.t;

                    if (length2(Point(inter_point) - Point(new_inter_point)) < length2(Point(inter_point) - Point(light_position)))
                    {
                        //We found an object that is between the light and the current inter_point: the point is shadowed

                        return true;
                    }
                }
            }
        }

        for (AnalyticShapesTypes analytic_shape : _analytic_shapes)
        {
            bool inter_found = false;

            std::visit([&] (auto& shape)
            {
                if (shape.intersect(ray, hitInfo))
                {
                    Point new_inter_point = ray._origin + ray._direction * hitInfo.t;

                    if (length2(Point(inter_point) - Point(new_inter_point)) < length2(Point(inter_point) - Point(light_position)))
                    {
                        //We found an object that is between the light and the current inter_point: the point is shadowed

                        inter_found = true;
                    }
                }
            }, analytic_shape);

            if (inter_found)
                return true;
        }

    }	//We haven't found any object between the light source and the intersection point, the point isn't shadowed

    return false;
}

Color Renderer::shade_abs_normals(const Vector& normalized_normal) const
{
    return Color(std::abs(normalized_normal.x), std::abs(normalized_normal.y), std::abs(normalized_normal.z));
}

Color Renderer::shade_pastel_normals(const Vector& normalized_normal) const
{
    return (Color(normalized_normal.x, normalized_normal.y, normalized_normal.z) + Color(1.0f, 1.0f, 1.0f)) * 0.5;
}

Color Renderer::shade_barycentric_coordinates(float u, float v) const
{
    return Color(1, 0, 0) * u + Color(0, 1.0, 0) * v + Color(0, 0, 1) * (1 - u - v);
}

Color Renderer::shade_visualize_ao(const Triangle& triangle, float u, float v) const
{
    Color color;

    color = Color(0.9f, 0.9f, 0.9f);//Almost pure white

    if (_render_settings.enable_ao_mapping)
    {
        float tex_coord_u, tex_coord_v;
        triangle.interpolate_texcoords(u, v, tex_coord_u, tex_coord_v);

        color = color * Color(sample_texture(_ao_map, tex_coord_u, tex_coord_v).r);
    }

    return color;
}

Color Renderer::shade_ray_inter_point(const Ray& ray, const HitInfo& hit_info) const
{
    Color final_color = Color(0.0f, 0.0f, 0.0f);

    if (_render_settings.shading_method == RenderSettings::ShadingMethod::RT_SHADING)
    {
        Point inter_point = ray._origin + ray._direction * hit_info.t;
        Vector direction_to_light = normalize(_scene._point_light._position - inter_point);
        Vector normal = hit_info.normal_at_intersection;

        Material hit_material = _materials(hit_info.mat_index);

        float ao_map_contribution = 1.0f;
        if (_render_settings.enable_ao_mapping)
        {
            float tex_coord_u, tex_coord_v;
            hit_info.triangle->interpolate_texcoords(hit_info.u, hit_info.v, tex_coord_u, tex_coord_v);

            ao_map_contribution = sample_texture(_ao_map, tex_coord_u, tex_coord_v).r;
        }

        Color diffuse_color;
        if (_render_settings.enable_diffuse_mapping)
        {
            float tex_coord_u, tex_coord_v;
            hit_info.triangle->interpolate_texcoords(hit_info.u, hit_info.v, tex_coord_u, tex_coord_v);

            diffuse_color = sample_texture(_diffuse_map, tex_coord_u, tex_coord_v);
        }
        else
            diffuse_color = compute_diffuse(hit_material, normal, direction_to_light);

        final_color = final_color + diffuse_color * ao_map_contribution * _render_settings.enable_diffuse;
        final_color = final_color + compute_specular(hit_material, ray._direction, normal, direction_to_light) * _render_settings.enable_specular;
        if (is_shadowed(inter_point, hit_info.normal_at_intersection, _scene._point_light._position))
            final_color = final_color * Color(Renderer::SHADOW_INTENSITY);
        final_color = final_color + hit_material.emission * _render_settings.enable_emissive;
        if (hit_material.reflection > 0.0f)
            final_color = (1 - hit_material.reflection) * final_color + compute_reflection(ray, hit_info) * hit_material.reflection;

        final_color = final_color + Renderer::AMBIENT_COLOR * hit_material.ambient_coeff * (1 - hit_material.reflection) * _render_settings.enable_ambient;
    }
    else if (_render_settings.shading_method == RenderSettings::ShadingMethod::ABS_NORMALS_SHADING)
        //Color triangles with std::abs(normal)
        final_color = shade_abs_normals(hit_info.normal_at_intersection);
    else if (_render_settings.shading_method == RenderSettings::ShadingMethod::PASTEL_NORMALS_SHADING)
        //Color triangles with (normal + 1) * 0.5
        final_color = shade_pastel_normals(hit_info.normal_at_intersection);
    else if (_render_settings.shading_method == RenderSettings::ShadingMethod::BARYCENTRIC_COORDINATES_SHADING)
        //Color triangles with barycentric coordinates
        final_color = shade_barycentric_coordinates(hit_info.u, hit_info.v);
    else if (_render_settings.shading_method == RenderSettings::ShadingMethod::VISUALIZE_AO)
        final_color = shade_visualize_ao(*hit_info.triangle, hit_info.u, hit_info.v);


    final_color.r = std::clamp(final_color.r, 0.0f, 1.0f);
    final_color.g = std::clamp(final_color.g, 0.0f, 1.0f);
    final_color.b = std::clamp(final_color.b, 0.0f, 1.0f);
    final_color.a = 1.0f;//We don't need alpha now so forcing it to 1
    return final_color;
}

Color Renderer::trace_triangle(const Ray& ray, const Triangle& triangle) const
{
    HitInfo hit_info;
    Color finalColor = Color(0, 0, 0);

    if (triangle.intersect(ray, hit_info))
        finalColor = shade_ray_inter_point(ray, hit_info);

    return finalColor;
}

template<int plane_index, int plane_sign>
inline bool is_inside(const vec4& vertex)
{
    return false;
}

template<>
inline bool is_inside<0, 1>(const vec4& vertex)
{
    return vertex.x < vertex.w;
}

template<>
inline bool is_inside<0, -1>(const vec4& vertex)
{
    return vertex.x > -vertex.w;
}

template<>
inline bool is_inside<1, 1>(const vec4& vertex)
{
    return vertex.y < vertex.w;
}

template<>
inline bool is_inside<1, -1>(const vec4& vertex)
{
    return vertex.y > -vertex.w;
}

template<>
inline bool is_inside<2, 1>(const vec4& vertex)
{
    return vertex.z < vertex.w;
}

template<>
inline bool is_inside<2, -1>(const vec4& vertex)
{
    return vertex.z > -vertex.w;
}

template<int plane_index, int plane_sign>
int Renderer::clip_triangles_to_plane(std::array<Triangle4, 12>& to_clip, int nb_triangles, std::array<Triangle4, 12>& out_clipped) const
{
    const static float CLIPPING_EPSILON = 0;

    int sum_inside;
    bool a_inside, b_inside, c_inside;

    int triangles_added = 0;//Used as an index in the out_clipped array to know where to store clipped triangles

    for (int triangle_index = 0; triangle_index < nb_triangles; triangle_index++)
    {
        Triangle4& triangle_4 = to_clip[triangle_index];

        a_inside = is_inside<plane_index, plane_sign>(triangle_4._a);
        b_inside = is_inside<plane_index, plane_sign>(triangle_4._b);
        c_inside = is_inside<plane_index, plane_sign>(triangle_4._c);

        sum_inside = a_inside + b_inside + c_inside;

        if (sum_inside == 3)//All vertices inside, nothing to clip. Keeping the triangle as is
            out_clipped[triangles_added++] = triangle_4;
        else if (sum_inside == 1)
        {
            vec4 inside_vertex, outside_1, outside_2;
            float u_texcoords[3];//{ inside_vertex, outside_1, outside_2 }
            float v_texcoords[3];//{ inside_vertex, outside_1, outside_2 }

            if (a_inside)
            {
                inside_vertex = triangle_4._a;
                outside_1 = triangle_4._b;
                outside_2 = triangle_4._c;

                u_texcoords[0] = triangle_4._tex_coords_u.x;
                u_texcoords[1] = triangle_4._tex_coords_u.y;
                u_texcoords[2] = triangle_4._tex_coords_u.z;

                v_texcoords[0] = triangle_4._tex_coords_v.x;
                v_texcoords[1] = triangle_4._tex_coords_v.y;
                v_texcoords[2] = triangle_4._tex_coords_v.z;
            }
            else if (b_inside)
            {
                inside_vertex = triangle_4._b;
                outside_1 = triangle_4._c;
                outside_2 = triangle_4._a;

                u_texcoords[0] = triangle_4._tex_coords_u.y;
                u_texcoords[1] = triangle_4._tex_coords_u.z;
                u_texcoords[2] = triangle_4._tex_coords_u.x;

                v_texcoords[0] = triangle_4._tex_coords_v.y;
                v_texcoords[1] = triangle_4._tex_coords_v.z;
                v_texcoords[2] = triangle_4._tex_coords_v.x;
            }
            else if (c_inside)
            {
                inside_vertex = triangle_4._c;
                outside_1 = triangle_4._a;
                outside_2 = triangle_4._b;

                u_texcoords[0] = triangle_4._tex_coords_u.z;
                u_texcoords[1] = triangle_4._tex_coords_u.x;
                u_texcoords[2] = triangle_4._tex_coords_u.y;

                v_texcoords[0] = triangle_4._tex_coords_v.z;
                v_texcoords[1] = triangle_4._tex_coords_v.x;
                v_texcoords[2] = triangle_4._tex_coords_v.y;
            }

            //We're going to create one new triangle
            float inside_dist_to_plane = (inside_vertex(plane_index) - inside_vertex.w * plane_sign);
            float outside_1_dist_to_plane = (outside_1(plane_index) - outside_1.w * plane_sign);
            float outside_2_dist_to_plane = (outside_2(plane_index) - outside_2.w * plane_sign);

            float tP1 = outside_1_dist_to_plane / (outside_1_dist_to_plane - inside_dist_to_plane);//distance from inside_vertex to the first clipping point in the direction of insde_vertex
            float tP2 = outside_2_dist_to_plane / (outside_2_dist_to_plane - inside_dist_to_plane);//distance from inside_vertex to the second clipping point in the direction of insde_vertex

            //Adding EPSILON to avoid artifacts on the edges of the image where triangles are clipped
            vec4 P1 = outside_1 + (tP1 - CLIPPING_EPSILON) * (inside_vertex - outside_1);
            vec4 P2 = outside_2 + (tP2 - CLIPPING_EPSILON) * (inside_vertex - outside_2);

            Point new_tex_coords_u = Point(u_texcoords[0], u_texcoords[1] + (tP1 - CLIPPING_EPSILON) * (u_texcoords[0] - u_texcoords[1]), u_texcoords[2] + (tP2 - CLIPPING_EPSILON) * (u_texcoords[0] - u_texcoords[2]));
            Point new_tex_coords_v = Point(v_texcoords[0], v_texcoords[1] + (tP1 - CLIPPING_EPSILON) * (v_texcoords[0] - v_texcoords[1]), v_texcoords[2] + (tP2 - CLIPPING_EPSILON) * (v_texcoords[0] - v_texcoords[2]));

            //Creating the new triangle
            out_clipped[triangles_added++] = Triangle4(inside_vertex, P1, P2, new_tex_coords_u, new_tex_coords_v);
        }
        else if (sum_inside == 2)
        {
            vec4 inside_1, inside_2, outside_vertex;
            float u_texcoords[3];//{ inside_1, inside_2, outside_vertex }
            float v_texcoords[3];//{ inside_1, inside_2, outside_vertex }

            if (!a_inside)
            {
                outside_vertex = triangle_4._a;
                inside_1 = triangle_4._b;
                inside_2 = triangle_4._c;

                u_texcoords[0] = triangle_4._tex_coords_u.y;
                u_texcoords[1] = triangle_4._tex_coords_u.z;
                u_texcoords[2] = triangle_4._tex_coords_u.x;

                v_texcoords[0] = triangle_4._tex_coords_v.y;
                v_texcoords[1] = triangle_4._tex_coords_v.z;
                v_texcoords[2] = triangle_4._tex_coords_v.x;
            }
            else if (!b_inside)
            {
                outside_vertex = triangle_4._b;
                inside_1 = triangle_4._c;
                inside_2 = triangle_4._a;

                u_texcoords[0] = triangle_4._tex_coords_u.z;
                u_texcoords[1] = triangle_4._tex_coords_u.x;
                u_texcoords[2] = triangle_4._tex_coords_u.y;

                v_texcoords[0] = triangle_4._tex_coords_v.z;
                v_texcoords[1] = triangle_4._tex_coords_v.x;
                v_texcoords[2] = triangle_4._tex_coords_v.y;
            }
            else if (!c_inside)
            {
                outside_vertex = triangle_4._c;
                inside_1 = triangle_4._a;
                inside_2 = triangle_4._b;

                u_texcoords[0] = triangle_4._tex_coords_u.x;
                u_texcoords[1] = triangle_4._tex_coords_u.y;
                u_texcoords[2] = triangle_4._tex_coords_u.z;

                v_texcoords[0] = triangle_4._tex_coords_v.x;
                v_texcoords[1] = triangle_4._tex_coords_v.y;
                v_texcoords[2] = triangle_4._tex_coords_v.z;
            }

            float inside_1_dist_to_plane = (inside_1(plane_index) - inside_1.w * plane_sign);
            float inside_2_dist_to_plane = (inside_2(plane_index) - inside_2.w * plane_sign);
            float outside_dist_to_plane = (outside_vertex(plane_index) - outside_vertex.w * plane_sign);

            //distance from the outside vertex the first clipping point in the direction of the inside vertex
            float tP1 = outside_dist_to_plane / (outside_dist_to_plane - inside_1_dist_to_plane);
            //distance from the outside vertex the second clipping point in the direction of the inside vertex
            float tP2 = outside_dist_to_plane / (outside_dist_to_plane - inside_2_dist_to_plane);

            //Adding EPSILON to avoid artifacts on the edges of the image where triangles are clipped
            vec4 P1 = outside_vertex + (tP1 - CLIPPING_EPSILON) * (inside_1 - outside_vertex);
            vec4 P2 = outside_vertex + (tP2 - CLIPPING_EPSILON) * (inside_2 - outside_vertex);

            Point new_tex_coords_u_1 = Point(u_texcoords[0], u_texcoords[1], u_texcoords[2] + (tP2 - CLIPPING_EPSILON) * (u_texcoords[1] - u_texcoords[2]));
            Point new_tex_coords_v_1 = Point(v_texcoords[0], v_texcoords[1], v_texcoords[2] + (tP2 - CLIPPING_EPSILON) * (v_texcoords[1] - v_texcoords[2]));
            Point new_tex_coords_u_2 = Point(u_texcoords[0], u_texcoords[2] + (tP2 - CLIPPING_EPSILON) * (u_texcoords[1] - u_texcoords[2]), u_texcoords[2] + (tP1 - CLIPPING_EPSILON) * (u_texcoords[0] - u_texcoords[2]));
            Point new_tex_coords_v_2 = Point(v_texcoords[0], v_texcoords[2] + (tP2 - CLIPPING_EPSILON) * (v_texcoords[1] - v_texcoords[2]), v_texcoords[2] + (tP1 - CLIPPING_EPSILON) * (v_texcoords[0] - v_texcoords[2]));

            //Creating the 2 new triangles
            out_clipped[triangles_added++] = Triangle4(inside_1, inside_2, P2, new_tex_coords_u_1, new_tex_coords_v_1);
            out_clipped[triangles_added++] = Triangle4(inside_1, P2, P1, new_tex_coords_u_2, new_tex_coords_v_2);
        }
    }

    return triangles_added;
}

int Renderer::clip_triangle(std::array<Triangle4, 12>& to_clip_triangles, std::array<Triangle4, 12>& clipped_triangles) const
{
    int nb_triangles = 1;

    if (_render_settings.enable_clipping)
    {
        nb_triangles = clip_triangles_to_plane<0, 1>(to_clip_triangles, nb_triangles, to_clip_triangles);//right plane
        nb_triangles = clip_triangles_to_plane<0, -1>(to_clip_triangles, nb_triangles, clipped_triangles);//left plane
        nb_triangles = clip_triangles_to_plane<1, 1>(clipped_triangles, nb_triangles, to_clip_triangles);//top plane
        nb_triangles = clip_triangles_to_plane<1, -1>(to_clip_triangles, nb_triangles, clipped_triangles);//bottom plane
        nb_triangles = clip_triangles_to_plane<2, 1>(clipped_triangles, nb_triangles, to_clip_triangles);//far plane
        nb_triangles = clip_triangles_to_plane<2, -1>(to_clip_triangles, nb_triangles, clipped_triangles);//near plane
    }
    else
        clipped_triangles[0] = to_clip_triangles[0];

    return nb_triangles;
}

float Renderer::matrix_transform_z(const Transform& m, const Point& vertex)
{
    float x = vertex.x, y = vertex.y, z = vertex.z;

    float zt= m.data()[2 * 4 + 0] * x + m.data()[2 * 4 + 1] * y + m.data()[2 * 4 + 2] * z + m.data()[2 * 4 + 3];        // dot(vec4(m[2]), vec4(p, 1))
    float wt= m.data()[3 * 4 + 0] * x + m.data()[3 * 4 + 1] * y + m.data()[3 * 4 + 2] * z + m.data()[3 * 4 + 3];        // dot(vec4(m[3]), vec4(p, 1))

    if (wt == 1.0f)
        return zt;
    else
        return zt / wt;
}

void Renderer::raster_trace()
{
    Transform perspective_projection = _scene._camera._perspective_proj_mat;
    Transform perspective_projection_inv = _scene._camera._perspective_proj_mat_inv;

    int render_width, render_height;
    get_render_width_height(_render_settings, render_width, render_height);

    const float render_height_scaling = 1.0f / render_height * 2;
    const float render_width_scaling = 1.0f / render_width * 2;

    std::array<Triangle4, 12> to_clip_triangles;
    std::array<Triangle4, 12> clipped_triangles;

#pragma omp parallel for schedule(dynamic) private(to_clip_triangles, clipped_triangles)
    for (int triangle_index = 0; triangle_index < _triangles.size(); triangle_index++)
    {
        Triangle& original_triangle = _triangles[triangle_index];//World space
        Triangle transformed_triangle = _scene._camera._world_to_camera_mat(original_triangle);

        vec4 a_clip_space = perspective_projection(vec4(transformed_triangle._a));
        vec4 b_clip_space = perspective_projection(vec4(transformed_triangle._b));
        vec4 c_clip_space = perspective_projection(vec4(transformed_triangle._c));

        to_clip_triangles[0] = Triangle4(a_clip_space, b_clip_space, c_clip_space, transformed_triangle._tex_coords_u, transformed_triangle._tex_coords_v);
        int nb_clipped = clip_triangle(to_clip_triangles, clipped_triangles);

        for (int clipped_triangle_index = 0; clipped_triangle_index < nb_clipped; clipped_triangle_index++)
        {
            Triangle4 clipped_triangle = clipped_triangles[clipped_triangle_index];
            Triangle clipped_triangle_NDC = Triangle(clipped_triangle, original_triangle._materialIndex, clipped_triangle._tex_coords_u, clipped_triangle._tex_coords_v);
            Triangle clipped_triangle_cam_space = perspective_projection_inv(clipped_triangle_NDC);

            Point a_image_plane = clipped_triangle_NDC._a;
            Point b_image_plane = clipped_triangle_NDC._b;
            Point c_image_plane = clipped_triangle_NDC._c;

            //We don't care about the z coordinate here
            float invTriangleArea = 1 / ((b_image_plane.x - a_image_plane.x) * (c_image_plane.y - a_image_plane.y) - (b_image_plane.y - a_image_plane.y) * (c_image_plane.x - a_image_plane.x));

            //Computing the bounding box of the triangle so that next, we only test pixels that are in the bounding box of the triangle
            float boundingMinX = std::min(a_image_plane.x, std::min(b_image_plane.x, c_image_plane.x));
            float boundingMinY = std::min(a_image_plane.y, std::min(b_image_plane.y, c_image_plane.y));
            float boundingMaxX = std::max(a_image_plane.x, std::max(b_image_plane.x, c_image_plane.x));
            float boundingMaxY = std::max(a_image_plane.y, std::max(b_image_plane.y, c_image_plane.y));
		
            int minXPixels = (int)((boundingMinX + 1) * 0.5 * render_width);
            int minYPixels = (int)((boundingMinY + 1) * 0.5 * render_height);
            int maxXPixels = (int)((boundingMaxX + 1) * 0.5 * render_width);
            int maxYPixels = (int)((boundingMaxY + 1) * 0.5 * render_height);

            minXPixels = std::max(minXPixels, 0);
            minYPixels = std::max(minYPixels, 0);
            maxXPixels = std::min(render_width - 1, maxXPixels);
            maxYPixels = std::min(render_height - 1, maxYPixels);

            float image_y = minYPixels * render_height_scaling - 1;

            float image_x_increment = render_width_scaling;
            float image_y_increment = render_height_scaling;
            for (int py = minYPixels; py <= maxYPixels; py++, image_y += image_y_increment)
            {
                float image_x = minXPixels * render_width_scaling - 1;
                for (int px = minXPixels; px <= maxXPixels; px++, image_x += image_x_increment)
                {
                    //NOTE If there are still issues with the clipping algorithm creating new points
                    //just a little over the edge of the view frustum, consider using a simple std::max(0, ...)
                    assert(px >= 0 && px < render_width);
                    assert(py >= 0 && py < render_height);

                    //Adding 0.5*increment to consider the center of the pixel
                    Point pixel_point(image_x + image_x_increment * 0.5f, image_y + image_y_increment * 0.5f, -1);

                    float u = Triangle::edge_function(pixel_point, c_image_plane, a_image_plane);
                    if (u < 0)
                        continue;

                    float v = Triangle::edge_function(pixel_point, a_image_plane, b_image_plane);
                    if (v < 0)
                        continue;

                    float w = Triangle::edge_function(pixel_point, b_image_plane, c_image_plane);
                    if (w < 0)
                        continue;

                    u *= invTriangleArea;
                    v *= invTriangleArea;
                    w *= invTriangleArea;

                    //Inverse projecting the z coordinates (because only the z coordinate is interesting here)
                    //of the vertices of the triangle back into camera space (from NDC)
                    //These will be used to interpolate the z coordinate at the "intersection" point
                    //We're using the clipped triangle but non-transformed by the camera matrix because
                    //we want our z-buffer to represent "true" depth, not the depth of the triangles that we
                    //transformed by the camera matrix just for rasterizing purposes
                    float clipped_triangle_world_space_a_z = matrix_transform_z(_scene._camera._camera_to_world_mat, perspective_projection_inv(clipped_triangle_NDC._a));
                    float clipped_triangle_world_space_b_z = matrix_transform_z(_scene._camera._camera_to_world_mat, perspective_projection_inv(clipped_triangle_NDC._b));
                    float clipped_triangle_world_space_c_z = matrix_transform_z(_scene._camera._camera_to_world_mat, perspective_projection_inv(clipped_triangle_NDC._c));

                    //Z coordinate of the point on the "real 3D" (not the triangle projected on the image plane) triangle
                    //by interpolating the z coordinates of the 3 vertices
                    //Interpolating the z coordinate is going to give a positive z. The bigger the z, the farther away the point
                    //on the triangle from the camera
                    float zTriangle = -1 / (1 / clipped_triangle_world_space_a_z * w + 1 / clipped_triangle_world_space_b_z * u + 1 / clipped_triangle_world_space_c_z * v);

                    if (zTriangle < _z_buffer(py, px))
                    {
                        _z_buffer(py, px) = zTriangle;

                        if (_render_settings.enable_ssao)
                            _normal_buffer(py, px) = original_triangle._normal;

                        Color final_color;
                        if (_render_settings.shading_method == RenderSettings::ShadingMethod::RT_SHADING)
                        {
                            final_color = trace_triangle(Ray(_scene._camera._position,
                                                             normalize(_scene._camera._camera_to_world_mat(perspective_projection_inv(pixel_point)) - _scene._camera._position)),
                                                            _scene._camera._camera_to_world_mat(clipped_triangle_cam_space));
                        }
                        else if (_render_settings.shading_method == RenderSettings::ShadingMethod::ABS_NORMALS_SHADING)
                            //Color triangles with std::abs(normal)
                            final_color = shade_abs_normals(normalize(original_triangle._normal));
                        else if (_render_settings.shading_method == RenderSettings::ShadingMethod::PASTEL_NORMALS_SHADING)
                            //Color triangles with (normal + 1) * 0.5
                            final_color = shade_pastel_normals(normalize(original_triangle._normal));
                        else if (_render_settings.shading_method == RenderSettings::ShadingMethod::BARYCENTRIC_COORDINATES_SHADING)
                            final_color = shade_barycentric_coordinates(u, v);
                        else if (_render_settings.shading_method == RenderSettings::ShadingMethod::VISUALIZE_AO)
                            final_color = shade_visualize_ao(perspective_projection_inv(clipped_triangle_NDC), u, v);

                        _image.setPixel(px, py, ImageUtils::gkit_color_to_Qt_ARGB32_uint(final_color));
                    }

                }
            }
        }
    }
}

Color Renderer::trace_ray(const Ray& ray, HitInfo& final_hit_info, bool& intersection_found) const
{
    HitInfo local_hit_info;

    if (_render_settings.enable_bvh)
    {
        if (_bvh.intersect(ray, local_hit_info))
            if (local_hit_info.t < final_hit_info.t || final_hit_info.t == -1)
                final_hit_info = local_hit_info;
    }
    else
    {
        for (const Triangle& triangle : _triangles)
            if (triangle.intersect(ray, local_hit_info))
                if (local_hit_info.t < final_hit_info.t || final_hit_info.t == -1)
                    final_hit_info = local_hit_info;
    }
    
    for (AnalyticShapesTypes analytic_shape : _analytic_shapes)
    {
        std::visit([&] (auto& shape)
        {
            if (shape.intersect(ray, local_hit_info))
                if (local_hit_info.t < final_hit_info.t || final_hit_info.t == -1)
                    final_hit_info = local_hit_info;
        }, analytic_shape);
    }

    if (final_hit_info.t > 0)//We found an intersection
    {
        intersection_found = true;

        Color final_color = shade_ray_inter_point(ray, final_hit_info);
        final_color.r = std::clamp(final_color.r, 0.0f, 1.0f);
        final_color.g = std::clamp(final_color.g, 0.0f, 1.0f);
        final_color.b = std::clamp(final_color.b, 0.0f, 1.0f);
        final_color.a = 1.0f;//We don't need alpha now so forcing it to 1

        return final_color;
    }
    else
    {
        if (_render_settings.enable_skysphere)
        {
            float u = 0.5 + std::atan2(-ray._direction.z, -ray._direction.x) / (2 * M_PI);
            float v = 0.5 + std::asin(-ray._direction.y) / M_PI;

            return sample_texture(_skysphere, u, v);
        }
        else if (_render_settings.enable_skybox)
            return _skybox.sample(ray._direction);
        else
            return Renderer::BACKGROUND_COLOR;
    }
}

void Renderer::ray_trace()
{
    int render_width, render_height;
    get_render_width_height(_render_settings, render_width, render_height);

    //If we're using SSAA, the image is going to be downscaled at the end of the
    //render process to apply the SSAA. This problem is that, to save memory,
    //the image buffer of this renderer is going to be overwritten with the dowscaled
    //image meaning that after one render using SSAA, the image buffer of this renderer
    //is too small to contain the render size (since it has been downscaled). We're thus
    //recreating the image
    if (_render_settings.enable_ssaa)
        _image = QImage(render_width, render_height, QImage::Format_ARGB32);

#pragma omp parallel for schedule(dynamic) collapse(2)
    for (int py = 0; py < render_height; py++)
    {
        //Adding 0.5 to consider the center of the pixel
        float y_world = ((float)py + 0.5f) / render_height * 2 - 1;

        for (int px = 0; px < render_width; px++)
        {
            //Adding 0.5 to consider the center of the pixel
            float x_world = ((float)px + 0.5f) / render_width * 2 - 1;

            Point image_plane_point_vs = _scene._camera._perspective_proj_mat_inv(Point(x_world, y_world, -1));//View space
            Point image_plane_point_ws = _scene._camera._camera_to_world_mat(image_plane_point_vs); //World space

            Point camera_position = _scene._camera._position;
            Vector ray_direction = normalize(image_plane_point_ws - camera_position);
            Ray ray(camera_position, ray_direction);

            bool intersection_found = false;
            HitInfo hit_info;

            Color pixel_color = trace_ray(ray, hit_info, intersection_found);
            if (intersection_found)
            {
                //Updating the z_buffer for post-processing operations that need it
                if (_render_settings.enable_ssao) {
                    _z_buffer(py, px) = -(ray._origin.z + ray._direction.z * hit_info.t);
                    _normal_buffer(py, px) = hit_info.normal_at_intersection;
                }
            }

            _image.setPixel(px, py, ImageUtils::gkit_color_to_Qt_ARGB32_uint(pixel_color));
        }
    }
}

void Renderer::post_process()
{
    if (_render_settings.enable_ssao)
        post_process_ssao_SIMD();
    if (_render_settings.enable_ssaa)
        apply_ssaa();
}

void Renderer::apply_ssaa()
{
    QImage downscaled_image;

    ImageUtils::downscale_image_qt_ARGB32(_image, downscaled_image, _render_settings.ssaa_factor);

    _image = downscaled_image;
}

void Renderer::post_process_ssao_scalar()
{
    int render_width, render_height;
    get_render_width_height(_render_settings, render_width, render_height);

    short int* ao_buffer = new short int[render_height * render_width];
    std::memset(ao_buffer, 0, sizeof(short int) * render_height * render_width);

    XorShiftGenerator rand_generator;
#pragma omp parallel private(rand_generator)
    {
        rand_generator = XorShiftGenerator(omp_get_thread_num() * 24183 + rand());
#pragma omp for
        for (int y = 0; y < render_height; y++)
        {
            for (int x = 0; x < render_width; x++)
            {
                //No information here, there's no pixel to shade: background at this pixel on the image
                if (_z_buffer(y, x) == INFINITY)
                    continue;

                float x_ndc = (float)x / render_width * 2 - 1;
                float y_ndc = (float)y / render_height * 2 - 1;

                float view_z = _z_buffer(y, x);
                float view_ray_x = x_ndc * _scene._camera._aspect_ratio * std::tan(radians(_scene._camera._fov / 2));
                float view_ray_y = y_ndc * std::tan(radians(_scene._camera._fov / 2));
                Point camera_space_point = Point(view_ray_x * view_z, view_ray_y * view_z, -view_z);

                Vector normal = normalize(_normal_buffer(y, x));

                short int pixel_occlusion = 0;
                for (int i = 0; i < _render_settings.ssao_sample_count; i++)
                {
                    float rand_x = rand_generator.get_rand_bilateral();
                    float rand_y = rand_generator.get_rand_bilateral();
                    float rand_z = rand_generator.get_rand_bilateral();

                    Point random_sample = Point(normalize(Vector(rand_x, rand_y, rand_z)));
                    random_sample = random_sample * (rand_generator.get_rand_lateral() + 0.0001f);
                    random_sample = random_sample * _render_settings.ssao_radius;
                    random_sample = random_sample + camera_space_point;
                    if (dot(random_sample - camera_space_point, normal) < 0)//The point is not in front of the normal
                        random_sample = random_sample + 2 * (camera_space_point - random_sample);

                    Point random_sample_ndc = _scene._camera._perspective_proj_mat(random_sample);
                    int random_point_pixel_x = (int)((random_sample_ndc.x + 1) * 0.5 * render_width);
                    int random_point_pixel_y = (int)((random_sample_ndc.y + 1) * 0.5 * render_height);

                    random_point_pixel_x = std::min(std::max(0, random_point_pixel_x), render_width - 1);
                    random_point_pixel_y = std::min(std::max(0, random_point_pixel_y), render_height - 1);

                    float sample_geometry_depth = -_z_buffer(random_point_pixel_y, random_point_pixel_x);

                    //Range check
                    if (std::abs(sample_geometry_depth - camera_space_point.z) > _render_settings.ssao_radius)
                        continue;
                    if (random_sample.z < sample_geometry_depth)
                        pixel_occlusion++;
                }

                ao_buffer[y * render_width + x] = pixel_occlusion;
            }
        }
    }

    //Blurring the AO
    int blur_size = 7;
    int half_blur_size = blur_size / 2;
#pragma omp parallel for
    for (int y = half_blur_size; y < render_height - half_blur_size; y++)
    {
        for (int x = half_blur_size; x < render_width - half_blur_size; x++)
        {
            if (_z_buffer(y, x) == INFINITY)//Background pixel, we're not blurring it
                continue;

            int sum = 0;
            for (int offset_y = -half_blur_size; offset_y <= half_blur_size; offset_y++)
                for (int offset_x = -half_blur_size; offset_x <= half_blur_size; offset_x++)
                    sum += ao_buffer[(y + offset_y) * render_width + x + offset_x];

            //Applying directly on the image
            float color_multiplier = 1 - ((float)sum / (float)(blur_size * blur_size) / (float)_render_settings.ssao_sample_count * (float)_render_settings.ssao_amount);
            QColor pixel_color = _image.pixelColor(x, y);
            _image.setPixelColor(x, y, QColor(pixel_color.red() * color_multiplier, pixel_color.green() * color_multiplier, pixel_color.blue() * color_multiplier));
        }
    }

    delete[] ao_buffer;
}

void Renderer::post_process_ssao_SIMD()
{
    std::cout << "in ssao, " << _render_settings.ssao_amount << ", " << _render_settings.ssao_sample_count << ", " << _render_settings.ssao_radius << std::endl;

    int render_width, render_height;
    get_render_width_height(_render_settings, render_width, render_height);

    int* ao_buffer = new int[render_height * render_width];
    std::memset(ao_buffer, 0, sizeof(int) * render_height * render_width);

    __m256 render_height_vec = _mm256_set1_ps((float)render_height);
    __m256 render_width_vec = _mm256_set1_ps((float)render_width);

    __m256 ones = _mm256_set1_ps(1);
    __m256 twos = _mm256_set1_ps(2);

    float fov_multiplier_value = (float)std::tan(_scene._camera._fov / 2 / 180 * M_PI);
    __m256 fov_multiplier = _mm256_set1_ps(fov_multiplier_value);

    __m256_XorShiftGenerator rand_generator;
    XorShiftGenerator rand_generator_scalar;
#pragma omp parallel private(rand_generator)
    {
        rand_generator = __m256_XorShiftGenerator(_mm256_set_epi32(omp_get_thread_num() * 24183 + rand(),
                                                                   omp_get_thread_num() * 24183 + rand(),
                                                                   omp_get_thread_num() * 24183 + rand(),
                                                                   omp_get_thread_num() * 24183 + rand(),
                                                                   omp_get_thread_num() * 24183 + rand(),
                                                                   omp_get_thread_num() * 24183 + rand(),
                                                                   omp_get_thread_num() * 24183 + rand(),
                                                                   omp_get_thread_num() * 24183 + rand()));

        rand_generator_scalar = XorShiftGenerator(omp_get_thread_num() * 24183 + rand());
#pragma omp for
        for (int y = 0; y < render_height; y++)
        {
            __m256 y_vec = _mm256_set1_ps((float)y);
            __m256 y_ndc = _mm256_div_ps(y_vec, render_height_vec);
            y_ndc = _mm256_mul_ps(y_ndc, twos);
            y_ndc = _mm256_sub_ps(y_ndc, ones);

            for (int x = 0; x < render_width; x += 8)
            {
                __m256 view_z = _mm256_loadu_ps(_z_buffer.row(y) + x);
                __m256 infinity = _mm256_set1_ps(INFINITY);
                __m256 infinity_mask = _mm256_cmp_ps(view_z, infinity, _CMP_NEQ_OQ);

                float sum = _mm256_reduction_ps(infinity_mask);
                if (sum == 0.0)//We have no _z_buffer information on any pixels i.e. all pixels are background pixels
                    continue;//Skipping all these pixels

                __m256 x_vec = _mm256_set1_ps((float)x);

                __m256 xs = _mm256_set_ps(7, 6, 5, 4, 3, 2, 1, 0);
                xs = _mm256_add_ps(xs, x_vec);

                __m256 x_ndc = _mm256_div_ps(xs, render_width_vec);
                x_ndc = _mm256_mul_ps(x_ndc, twos);
                x_ndc = _mm256_sub_ps(x_ndc, ones);

                __m256 view_ray_x = _mm256_mul_ps(x_ndc, _mm256_mul_ps(fov_multiplier, _mm256_set1_ps(_scene._camera._aspect_ratio)));
                __m256 view_ray_y = _mm256_mul_ps(y_ndc, fov_multiplier);

                __m256Point camera_space_point = __m256Point(_mm256_mul_ps(view_z, view_ray_x), _mm256_mul_ps(view_z, view_ray_y), _mm256_mul_ps(view_z, _mm256_set1_ps(-1)));

                __m256Vector normal = _mm256_normalize(__m256Vector(_normal_buffer.row(y) + x));

                __m256i pixel_occlusion = _mm256_setzero_si256();
                for (int i = 0; i < _render_settings.ssao_sample_count; i++)
                {
                    __m256 rand_x = rand_generator.get_rand_bilateral();
                    __m256 rand_y = rand_generator.get_rand_bilateral();
                    __m256 rand_z = rand_generator.get_rand_bilateral();

                    __m256Point random_sample = __m256Point(_mm256_normalize(__m256Vector(rand_x, rand_y, rand_z)));
                    random_sample = random_sample * _mm256_add_ps(rand_generator.get_rand_lateral(), _mm256_set1_ps(0.0001f));
                    random_sample = random_sample * _mm256_set1_ps(_render_settings.ssao_radius);
                    random_sample = random_sample + camera_space_point;

                    //Flipping the random smaple if the dot product with the normal is negative, i.e., the sample
                    //is below the surface of the hemisphere and we're thus flipping the point back into the hemisphere
                    //IN FRONT of the normal
                    __m256Vector vec_dot = random_sample - camera_space_point;
                    __m256 dot_result = _mm256_dot_product(vec_dot, normal);
                    __m256 dot_mask = _mm256_and_ps(_mm256_cmp_ps(dot_result, _mm256_setzero_ps(), _CMP_LT_OQ), ones);
                    __m256Vector backflip_vec = 2 * (camera_space_point - random_sample);

                    //+ 2 * (camera_space_point - random_sample) if dot < 0
                    random_sample = random_sample + backflip_vec * dot_mask;

                    __m256Point random_sample_ndc = random_sample.transform(_scene._camera._perspective_proj_mat);

                    __m256 zero_point_five = _mm256_set1_ps(0.5);
                    __m256i random_point_pixel_x = _mm256_cvtps_epi32(_mm256_mul_ps(_mm256_mul_ps(_mm256_add_ps(random_sample_ndc._x, ones), zero_point_five), render_width_vec));
                    __m256i random_point_pixel_y = _mm256_cvtps_epi32(_mm256_mul_ps(_mm256_mul_ps(_mm256_add_ps(random_sample_ndc._y, ones), zero_point_five), render_height_vec));

                    //Clamping between 0 and render_width - 1
                    random_point_pixel_x = _mm256_min_epi32(random_point_pixel_x, _mm256_cvtps_epi32(_mm256_sub_ps(render_width_vec, ones)));
                    random_point_pixel_x = _mm256_max_epi32(random_point_pixel_x, _mm256_set1_epi32(0));

                    random_point_pixel_y = _mm256_min_epi32(random_point_pixel_y, _mm256_cvtps_epi32(_mm256_sub_ps(render_height_vec, ones)));
                    random_point_pixel_y = _mm256_max_epi32(random_point_pixel_y, _mm256_set1_epi32(0));

                    __m256i z_buffer_address_offset = random_point_pixel_x;
                    z_buffer_address_offset = _mm256_add_epi32(z_buffer_address_offset, _mm256_mullo_epi32(random_point_pixel_y, _mm256_cvtps_epi32(render_width_vec)));

                    __m256 sample_geometry_depth = _mm256_mul_ps(_mm256_set1_ps(-1.0f), _mm256_i32gather_ps(_z_buffer.data(), z_buffer_address_offset, sizeof(float)));

                    __m256 occluded_mask = _mm256_set1_ps(1);

                    //Range check
                    __m256 absolute_value_range_check = _mm256_andnot_ps(_mm256_set1_ps(-0.0f), _mm256_sub_ps(sample_geometry_depth, camera_space_point._z));
                    __m256 range_check_mask = _mm256_cmp_ps(absolute_value_range_check, _mm256_set1_ps(_render_settings.ssao_radius), _CMP_LE_OQ);
                    occluded_mask = _mm256_and_ps(occluded_mask, range_check_mask);

                    __m256 sample_depth_mask = _mm256_cmp_ps(random_sample._z, sample_geometry_depth, _CMP_LT_OQ);
                    occluded_mask = _mm256_and_ps(occluded_mask, sample_depth_mask);

                    //Some of the 8 pixels being processed may not have _z_buffer information. i.e. some 
                    //of the pixels being processed are background pixels. We're going to mask those out
                    //to avoid occluding them
                    occluded_mask = _mm256_and_ps(occluded_mask, infinity_mask);

                    pixel_occlusion = _mm256_add_epi32(pixel_occlusion, _mm256_cvtps_epi32(occluded_mask));
                }

                _mm256_storeu_si256((__m256i*) & ao_buffer[y * render_width + x], pixel_occlusion);
            }

            int leftover = render_width % 8;
            for (int x = render_width - leftover; x < render_width; x++)
            {
                //No information here, there's no pixel to shade: background at this pixel on the image
                if (_z_buffer(y, x) == INFINITY)
                    continue;

                float x_ndc = (float)x / render_width * 2 - 1;
                float y_ndc = (float)y / render_height * 2 - 1;

                float view_z = _z_buffer(y, x);
                float view_ray_x = x_ndc * _scene._camera._aspect_ratio * std::tan(radians(_scene._camera._fov / 2));
                float view_ray_y = y_ndc * std::tan(radians(_scene._camera._fov / 2));
                Point camera_space_point = Point(view_ray_x * view_z, view_ray_y * view_z, -view_z);

                Vector normal = normalize(_normal_buffer(y, x));

                short int pixel_occlusion = 0;
                for (int i = 0; i < _render_settings.ssao_sample_count; i++)
                {
                    float rand_x = rand_generator_scalar.get_rand_bilateral();
                    float rand_y = rand_generator_scalar.get_rand_bilateral();
                    float rand_z = rand_generator_scalar.get_rand_bilateral();

                    Point random_sample = Point(normalize(Vector(rand_x, rand_y, rand_z)));
                    random_sample = random_sample * (rand_generator_scalar.get_rand_lateral() + 0.0001f);
                    random_sample = random_sample * _render_settings.ssao_radius;
                    random_sample = random_sample + camera_space_point;
                    if (dot(random_sample - camera_space_point, normal) < 0)//The point is not in front of the normal
                        random_sample = random_sample + 2 * (camera_space_point - random_sample);

                    Point random_sample_ndc = _scene._camera._perspective_proj_mat(random_sample);
                    int random_point_pixel_x = (int)((random_sample_ndc.x + 1) * 0.5 * render_width);
                    int random_point_pixel_y = (int)((random_sample_ndc.y + 1) * 0.5 * render_height);

                    random_point_pixel_x = std::min(std::max(0, random_point_pixel_x), render_width - 1);
                    random_point_pixel_y = std::min(std::max(0, random_point_pixel_y), render_height - 1);

                    float sample_geometry_depth = -_z_buffer(random_point_pixel_y, random_point_pixel_x);

                    //Range check
                    if (std::abs(sample_geometry_depth - camera_space_point.z) > _render_settings.ssao_radius)
                        continue;
                    if (random_sample.z < sample_geometry_depth)
                        pixel_occlusion++;
                }

                ao_buffer[y * render_width + x] = pixel_occlusion;
            }
        }
    }

    //Blurring the AO
    int blur_size = 7;
    int half_blur_size = blur_size / 2;
#pragma omp parallel for
    for (int y = half_blur_size; y < render_height - half_blur_size; y++)
    {
        for (int x = half_blur_size; x < render_width - half_blur_size; x++)
        {
            if (_z_buffer(y, x) == INFINITY)//Background pixel, we're not blurring it
                continue;

            int sum = 0;
            for (int offset_y = -half_blur_size; offset_y <= half_blur_size; offset_y++)
                for (int offset_x = -half_blur_size; offset_x <= half_blur_size; offset_x++)
                    sum += ao_buffer[(y + offset_y) * render_width + x + offset_x];

            //Applying directly on the image
            float color_multiplier = 1 - ((float)sum / (float)(blur_size * blur_size) / (float)_render_settings.ssao_sample_count * (float)_render_settings.ssao_amount);
            QColor pixel_color = _image.pixelColor(x, y);
            _image.setPixelColor(x, y, QColor(pixel_color.red() * color_multiplier, pixel_color.green() * color_multiplier, pixel_color.blue() * color_multiplier));
        }
    }

    delete[] ao_buffer;
}
