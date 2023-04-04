#include "m256Point.h"
#include "m256Vector.h"
#include "mat.h"
#include "renderer.h"
#include "timer.h"
#include "xorshift.h"

#include <cmath>
#include <cstring>
#include <immintrin.h>
#include <omp.h>

#ifndef M_PI
    #define M_PI 3.141592653589793
#endif

Material init_default_material()
{
    Material mat = Material(Color(1.0f, 0.0f, 0.5f));//Pink color
    mat.specular = Color(1.0f, 1.0f, 1.0f);
    mat.ns = 15;

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
Material Renderer::DEBUG_MATERIAL_1 = init_debug_material_1();
Color Renderer::AMBIENT_COLOR = Color(0.1f, 0.1f, 0.1f);
Color Renderer::BACKGROUND_COLOR = Color(135.0f / 255.0f, 206.0f / 255.0f, 235.0f / 255.0f);//Sky color

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

Image* Renderer::getImage()
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
    {
        _z_buffer = Buffer<float>(width, height);
        _z_buffer.fill_values(INFINITY);
    }

    if (_render_settings.enable_ssao)
        _normal_buffer = Buffer<Vector>(width, height);

    _image = Image(width, height);
    for (int i = 0; i < height; i++)
        for (int j = 0; j < width; j++)
            _image(j, i) = Renderer::BACKGROUND_COLOR;
}

Color Renderer::computeDiffuse(const Material& hitMaterial, const Vector& normal, const Vector& direction_to_light) const
{
    return hitMaterial.diffuse * std::max(0.0f, dot(normal, direction_to_light));
}

Color Renderer::computeSpecular(const Material& hit_material, const Vector& ray_direction, const Vector& normal, const Vector& direction_to_light) const
{
    Vector incident_direction = -direction_to_light;
    Vector reflection_ray = incident_direction - 2 * dot(normal, incident_direction) * normal;
    float angle = dot(reflection_ray, -ray_direction);

    //Specular optimization to avoid computing the exponentiation when not necessary (i.e. when it corresponds to a negligeable visual impact)
    if (angle <= hit_material.specular_threshold)//We're below the visibility threshold so we're not going to notice the specular anyway, returning no specular
        return Color(0, 0, 0);
    else
        return hit_material.specular * std::pow(std::max(0.0f, angle), hit_material.ns);
}

bool Renderer::is_shadowed(const Point& inter_point, const Point& light_position) const
{
    if (_render_settings.compute_shadows)
    {
        Ray ray(inter_point, light_position - inter_point);
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


    }	//We haven't found any object between the light source and the intersection point, the point isn't shadowed

    return false;
}

void Renderer::set_triangles(std::vector<Triangle> triangles)
{
    _triangles = triangles;

    if (_render_settings.enable_bvh)
        _bvh = BVH(&_triangles, _render_settings.bvh_max_depth, _render_settings.bvh_leaf_object_count);
}

void Renderer::set_materials(Materials materials)
{
    _materials = materials;
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
            _image(j, i) = Renderer::BACKGROUND_COLOR;
}

void Renderer::change_camera_fov(float fov)
{
    _scene._camera.set_fov(fov);
}

void Renderer::change_camera_aspect_ratio(float aspect_ratio)
{
    _scene._camera.set_aspect_ratio(aspect_ratio);
}

void Renderer::change_render_size(int width, int height)
{
    _render_settings.image_width = width;
    _render_settings.image_height = height;

    int render_width, render_height;
    get_render_width_height(_render_settings, render_width, render_height);

    init_buffers(render_width, render_height);

    _scene._camera.set_aspect_ratio((float)render_width / render_height);
}

Color Renderer::trace_triangle(const Ray& ray, const Triangle& triangle) const
{
    HitInfo hit_info;
    Color finalColor = Color(0, 0, 0);

    if (triangle.intersect(ray, hit_info))
    {
        if (_render_settings.use_shading)
        {
            Point inter_point = ray._origin + ray._direction * (hit_info.t + Renderer::EPSILON);
            Vector direction_to_light = normalize(_scene._point_light._position - inter_point);
            Vector normal = normalize(hit_info.normal_at_intersection);

            Material hit_material;
            if (hit_info.mat_index == -1)
                hit_material = Renderer::DEFAULT_MATERIAL;
            else if (hit_info.mat_index == -2)//Debug material
                hit_material = Renderer::DEBUG_MATERIAL_1;
            else
                hit_material = _materials(hit_info.mat_index);

            finalColor = finalColor + computeDiffuse(hit_material, normal, direction_to_light);
            finalColor = finalColor + computeSpecular(hit_material, ray._direction, normal, direction_to_light);
            if (is_shadowed(inter_point, _scene._point_light._position))
                finalColor = finalColor * Color(Renderer::SHADOW_INTENSITY);
            finalColor = finalColor + hit_material.emission;

            finalColor = finalColor + Renderer::AMBIENT_COLOR * hit_material.ambient_coeff;
        }
        else
        {
            if (_render_settings.color_normal_or_barycentric)
            {
                //Color triangles with normal
                Vector normalized_normal = normalize(hit_info.normal_at_intersection);
                finalColor = Color(std::abs(normalized_normal.x), std::abs(normalized_normal.y), std::abs(normalized_normal.z));
            }
            else
            {
                //Color triangles with barycentric coordinates
                float u = hit_info.u;
                float v = hit_info.v;

                finalColor = Color(1, 0, 0) * u + Color(0, 1.0, 0) * v + Color(0, 0, 1) * (1 - u - v);
            }
        }
    }

    return finalColor;
}

//TODO factoriser tout ça en seulement 6 fonctions en prenant directement le vertex qui va bien en paramètre plut$ot que le triangle en entier
template<int plane_index, int plane_sign>
inline bool is_a_inside(const Triangle4& triangle)
{
    return false;
}

template<int plane_index, int plane_sign>
inline bool is_b_inside(const Triangle4& triangle)
{
    return false;
}

template<int plane_index, int plane_sign>
inline bool is_c_inside(const Triangle4& triangle)
{
    return false;
}

template<>
inline bool is_a_inside<0, 1>(const Triangle4& triangle)
{
    return triangle._a.x < triangle._a.w;
}

template<>
inline bool is_a_inside<0, -1>(const Triangle4& triangle)
{
    return triangle._a.x > -triangle._a.w;
}

template<>
inline bool is_a_inside<1, 1>(const Triangle4& triangle)
{
    return triangle._a.y < triangle._a.w;
}

template<>
inline bool is_a_inside<1, -1>(const Triangle4& triangle)
{
    return triangle._a.y > -triangle._a.w;
}

template<>
inline bool is_a_inside<2, 1>(const Triangle4& triangle)
{
    return triangle._a.z < triangle._a.w;
}

template<>
inline bool is_a_inside<2, -1>(const Triangle4& triangle)
{
    return triangle._a.z > -triangle._a.w;
}




template<>
inline bool is_b_inside<0, 1>(const Triangle4& triangle)
{
    return triangle._b.x < triangle._b.w;
}

template<>
inline bool is_b_inside<0, -1>(const Triangle4& triangle)
{
    return triangle._b.x > -triangle._b.w;
}

template<>
inline bool is_b_inside<1, 1>(const Triangle4& triangle)
{
    return triangle._b.y < triangle._b.w;
}

template<>
inline bool is_b_inside<1, -1>(const Triangle4& triangle)
{
    return triangle._b.y > -triangle._b.w;
}

template<>
inline bool is_b_inside<2, 1>(const Triangle4& triangle)
{
    return triangle._b.z < triangle._b.w;
}

template<>
inline bool is_b_inside<2, -1>(const Triangle4& triangle)
{
    return triangle._b.z > -triangle._b.w;
}



template<>
inline bool is_c_inside<0, 1>(const Triangle4& triangle)
{
    return triangle._c.x < triangle._c.w;
}

template<>
inline bool is_c_inside<0, -1>(const Triangle4& triangle)
{
    return triangle._c.x > -triangle._c.w;
}

template<>
inline bool is_c_inside<1, 1>(const Triangle4& triangle)
{
    return triangle._c.y < triangle._c.w;
}

template<>
inline bool is_c_inside<1, -1>(const Triangle4& triangle)
{
    return triangle._c.y > -triangle._c.w;
}

template<>
inline bool is_c_inside<2, 1>(const Triangle4& triangle)
{
    return triangle._c.z < triangle._c.w;
}

template<>
inline bool is_c_inside<2, -1>(const Triangle4& triangle)
{
    return triangle._c.z > -triangle._c.w;
}

template<int plane_index, int plane_sign>
int Renderer::clip_triangles_to_plane(std::array<Triangle4, 12>& to_clip, int nb_triangles, std::array<Triangle4, 12>& out_clipped) const
{
    const static float CLIPPING_EPSILON = 1.0e-5f;

    int sum_inside;
    bool a_inside, b_inside, c_inside;

    int triangles_added = 0;//Used as an index in the out_clipped array to know where to store clipped triangles

    for (int triangle_index = 0; triangle_index < nb_triangles; triangle_index++)
    {
        Triangle4& triangle_4 = to_clip[triangle_index];

        a_inside = is_a_inside<plane_index, plane_sign>(triangle_4);
        b_inside = is_b_inside<plane_index, plane_sign>(triangle_4);
        c_inside = is_c_inside<plane_index, plane_sign>(triangle_4);

        sum_inside = a_inside + b_inside + c_inside;

        if (sum_inside == 3)//All vertices inside, nothing to clip. Keeping the triangle as is
            out_clipped[triangles_added++] = triangle_4;
        else if (sum_inside == 1)
        {
            vec4 inside_vertex, outside_1, outside_2;
            if (a_inside)
            {
                inside_vertex = triangle_4._a;
                outside_1 = triangle_4._b;
                outside_2 = triangle_4._c;
            }
            else if (b_inside)
            {
                inside_vertex = triangle_4._b;
                outside_1 = triangle_4._c;
                outside_2 = triangle_4._a;
            }
            else if (c_inside)
            {
                inside_vertex = triangle_4._c;
                outside_1 = triangle_4._a;
                outside_2 = triangle_4._b;
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

            //Creating the new triangle
            out_clipped[triangles_added++] = Triangle4(inside_vertex, P1, P2);
        }
        else if (sum_inside == 2)
        {
            vec4 inside_1, inside_2, outside_vertex;
            if (!a_inside)
            {
                outside_vertex = triangle_4._a;
                inside_1 = triangle_4._b;
                inside_2 = triangle_4._c;
            }
            else if (!b_inside)
            {
                outside_vertex = triangle_4._b;
                inside_1 = triangle_4._c;
                inside_2 = triangle_4._a;
            }
            else if (!c_inside)
            {
                outside_vertex = triangle_4._c;
                inside_1 = triangle_4._a;
                inside_2 = triangle_4._b;
            }

            float inside_1_dist_to_plane = (inside_1(plane_index) - inside_1.w * plane_sign);
            float inside_2_dist_to_plane = (inside_2(plane_index) - inside_2.w * plane_sign);
            float outside_dist_to_plane = (outside_vertex(plane_index) - outside_vertex.w * plane_sign);

            //distance from the outside vertex the first clipping point the direction of the inside vertex
            float tP1 = outside_dist_to_plane / (outside_dist_to_plane - inside_1_dist_to_plane);
            //distance from the outside vertex the second clipping point the direction of the inside vertex
            float tP2 = outside_dist_to_plane / (outside_dist_to_plane - inside_2_dist_to_plane);

            //Adding EPSILON to avoid artifacts on the edges of the image where triangles are clipped
            vec4 P1 = outside_vertex + (tP1 - CLIPPING_EPSILON) * (inside_1 - outside_vertex);
            vec4 P2 = outside_vertex + (tP2 - CLIPPING_EPSILON) * (inside_2 - outside_vertex);

            //Creating the 2 new triangles
            out_clipped[triangles_added++] = Triangle4(inside_1, inside_2, P2);
            out_clipped[triangles_added++] = Triangle4(inside_1, P2, P1);
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

//#pragma omp parallel for schedule(dynamic) private(to_clip_triangles, clipped_triangles)
    for (int triangle_index = 0; triangle_index < _triangles.size(); triangle_index++)
    {
        Triangle& triangle = _triangles[triangle_index];

        vec4 a_clip_space = perspective_projection(vec4(triangle._a));
        vec4 b_clip_space = perspective_projection(vec4(triangle._b));
        vec4 c_clip_space = perspective_projection(vec4(triangle._c));

        to_clip_triangles[0] = Triangle4(a_clip_space, b_clip_space, c_clip_space);
        int nb_clipped = clip_triangle(to_clip_triangles, clipped_triangles);

        for (int clipped_triangle_index = 0; clipped_triangle_index < nb_clipped; clipped_triangle_index++)
        {
            Triangle4 clipped_triangle = clipped_triangles[clipped_triangle_index];
            Triangle clipped_triangle_NDC = Triangle(clipped_triangle, triangle._materialIndex);


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

            //minXPixels = std::max(0, minXPixels);//TODO remove
            //minYPixels = std::max(0, minYPixels);//TODO remove
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
                    Point pixel_point(image_x + image_x_increment * 0.5, image_y + image_y_increment * 0.5, -1);

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

                    //Inverse projecting the triangle back into camera space to interpolate the z coordinate correctly
                    Triangle clipped_triangle_camera_space = perspective_projection_inv(clipped_triangle_NDC);
                    //TODO opti ça pour ne pas recalculer le x et le y dont on a pas besoin, on veut juste la coordonnées z projetée, pas les autres donc on peut s'éviter des calculs
//                    float near = _scene._camera._near, far = _scene._camera._far;
//                    float id= 1 / (near - far);
//                    float y_x_term = perspective_projection_inv.data()[2 * 4 + 0];
//                    float y_y_term = perspective_projection_inv.data()[2 * 4 + 1];
//                    float y_z_term = perspective_projection_inv.data()[2 * 4 + 2];
//                    float y_w_term = perspective_projection_inv.data()[2 * 4 + 3];//w term of the matrix/point multiplication when projecting
//                    float w_x_term = perspective_projection_inv.data()[3 * 4 + 0];
//                    float w_y_term = perspective_projection_inv.data()[3 * 4 + 1];
//                    float w_z_term = perspective_projection_inv.data()[3 * 4 + 2];
//                    float w_w_term = perspective_projection_inv.data()[3 * 4 + 3];
//                    float z_a = (clipped_triangle_NDC._a.x * y_x_term + clipped_triangle_NDC._a.y * y_y_term + clipped_triangle_NDC._a.z * y_z_term + y_w_term) / (clipped_triangle_NDC._a.x * w_x_term + clipped_triangle_NDC._a.y * w_y_term + clipped_triangle_NDC._a.z * w_z_term + w_w_term);
//                    float z_b = (clipped_triangle_NDC._b.x * y_x_term + clipped_triangle_NDC._b.y * y_y_term + clipped_triangle_NDC._b.z * y_z_term + y_w_term) / (clipped_triangle_NDC._b.x * w_x_term + clipped_triangle_NDC._b.y * w_y_term + clipped_triangle_NDC._b.z * w_z_term + w_w_term);
//                    float z_c = (clipped_triangle_NDC._c.x * y_x_term + clipped_triangle_NDC._c.y * y_y_term + clipped_triangle_NDC._c.z * y_z_term + y_w_term) / (clipped_triangle_NDC._c.x * w_x_term + clipped_triangle_NDC._c.y * w_y_term + clipped_triangle_NDC._c.z * w_z_term + w_w_term);

                    //std::cout << perspective_projection(clipped_triangle_NDC)._a.z << ", " << perspective_projection(clipped_triangle_NDC)._b.z << ", " << perspective_projection(clipped_triangle_NDC)._c.z << ", " << std::endl;
                    //std::cout << perspective_projection_inv(clipped_triangle_NDC)._a.z << ", " << perspective_projection_inv(clipped_triangle_NDC)._b.z << ", " << perspective_projection_inv(clipped_triangle_NDC)._c.z << ", " << std::endl;
                    //std::cout << "z: " << z_a << ", " << z_b << ", " << z_c << std::endl;
                    //std::exit(0);
                    //std::cout << perspective_projection.data()[2 * 4] << ", " << perspective_projection.data()[2 * 4 + 1] << ", " << perspective_projection.data()[2 * 4 + 2] << ", " << perspective_projection.data()[2 * 4 + 3] << std::endl << std::endl;
                    //std::cout << perspective_projection_inv.data()[2 * 4] << ", " << perspective_projection_inv.data()[2 * 4 + 1] << ", " << perspective_projection_inv.data()[2 * 4 + 2] << ", " << perspective_projection_inv.data()[2 * 4 + 3] << std::endl << std::endl;
                    //std::exit(0);

                    //Z coordinate of the point on the "real 3D" (not the triangle projected on the image plane) triangle
                    //by interpolating the z coordinates of the 3 vertices
                    //Interpolating the z coordinate is going to give a positive z. The bigger the z, the farther away the point
                    //on the triangle from the camera
                    float zTriangle = -1 / (1 / clipped_triangle_camera_space._a.z * w + 1 / clipped_triangle_camera_space._b.z * u + 1 / clipped_triangle_camera_space._c.z * v);

                    //TODO lundi 02/04: debug z-buffer
                    if (zTriangle < _z_buffer(py, px))
                    {
                        //buffer before: -5.09607	buffer after: -5.10603
                        _z_buffer(py, px) = zTriangle;

                        if (_render_settings.enable_ssao)
                            _normal_buffer(py, px) = triangle._normal;

                        Color final_color;
                        if (_render_settings.use_shading)
                            final_color = trace_triangle(Ray(_scene._camera._position, normalize(perspective_projection_inv(pixel_point) - _scene._camera._position)), perspective_projection_inv(clipped_triangle_NDC));
                        else if (_render_settings.color_normal_or_barycentric)
                        {
                            Vector normalized_normal = normalize(cross(triangle._b - triangle._a, triangle._c - triangle._a));
                            final_color = Color(std::abs(normalized_normal.x), std::abs(normalized_normal.y), std::abs(normalized_normal.z));
                        }
                        else //Color triangles with barycentric coordinates
                            final_color = Color(1, 0, 0) * u + Color(0, 1.0, 0) * v + Color(0, 0, 1) * (1 - u - v);

                        _image(px, py) = final_color;
                    }

                }
            }
        }
    }
}

void Renderer::ray_trace()
{
    int render_width, render_height;
    get_render_width_height(_render_settings, render_width, render_height);

    float fov_scaling = tan(radians(_scene._camera._fov / 2));

#pragma omp parallel for schedule(dynamic)
    for (int py = 0; py < render_height; py++)
    {
        //Adding 0.5 to consider the center of the pixel
        float y_world = ((float)py + 0.5) / render_height * 2 - 1;
        y_world *= fov_scaling;

        for (int px = 0; px < render_width; px++)
        {
            //Adding 0.5 to consider the center of the pixel
            float x_world = ((float)px + 0.5) / render_width * 2 - 1;
            x_world *= (float)render_width / render_height;
            x_world *= fov_scaling;

            Point image_plane_point = Point(x_world, y_world, -1);

            Point camera_position = _scene._camera._position;
            Vector ray_direction = normalize(image_plane_point - camera_position);
            Ray ray(camera_position, ray_direction);

            HitInfo finalHitInfo;
            HitInfo hit_info;

            //TODO mettre un std::cout << dans le copy constructor des Triangle pour voir partout où le copy constructor est appelé
            if (!_render_settings.enable_bvh)//TODO remove !
            {
                if (_bvh.intersect(ray, hit_info))
                    if (hit_info.t < finalHitInfo.t || finalHitInfo.t == -1)
                        finalHitInfo = hit_info;
            }
            else
            {
                std::array<Triangle4, 12> to_clip_triangles;
                std::array<Triangle4, 12> clipped_triangles;

                for (Triangle& triangle : _triangles)
                    if (triangle.intersect(ray, hit_info))
                        if (hit_info.t < finalHitInfo.t || finalHitInfo.t == -1)
                            finalHitInfo = hit_info;
            }

            Color finalColor;

            if (finalHitInfo.t > 0)//We found an intersection
            {
                //Updating the z_buffer for post-processing operations that need it
                if (_render_settings.enable_ssao) {
                    _z_buffer(py, px) = -(ray._origin.z + ray._direction.z * finalHitInfo.t);
                    _normal_buffer(py, px) = finalHitInfo.normal_at_intersection;
                }

                //TODO factoriser ça dans une fonction parce que ce morcaeu de code où on teste (if shading) ... on le fait partout à plein d'endroit donc l'idée serait de faire une fonction qui fait tout ça pour nous
                if (_render_settings.use_shading)
                {
                    Point inter_point = ray._origin + ray._direction * (finalHitInfo.t + Renderer::EPSILON);
                    Vector direction_to_light = normalize(_scene._point_light._position - inter_point);
                    Vector normal = normalize(finalHitInfo.normal_at_intersection);

                    Material hit_material;
                    if (finalHitInfo.mat_index == -1)
                        hit_material = Renderer::DEFAULT_MATERIAL;
                    else
                        hit_material = _materials(finalHitInfo.mat_index);

                    finalColor = finalColor + computeDiffuse(hit_material, normal, direction_to_light);
                    finalColor = finalColor + computeSpecular(hit_material, ray._direction, normal, direction_to_light);
                    finalColor = finalColor + hit_material.emission;
                    if (is_shadowed(inter_point, _scene._point_light._position))
                        finalColor = finalColor * Color(Renderer::SHADOW_INTENSITY);

                    finalColor = finalColor + Renderer::AMBIENT_COLOR * hit_material.ambient_coeff;
                }
                else if (_render_settings.color_normal_or_barycentric) //Color triangles with normal
                {
                    Vector normalized_normal = normalize(finalHitInfo.normal_at_intersection);
                    finalColor = Color(std::abs(normalized_normal.x), std::abs(normalized_normal.y), std::abs(normalized_normal.z));
                }
                else //Color triangles with barycentric coordinates
                {
                    float u = finalHitInfo.u;
                    float v = finalHitInfo.v;

                    finalColor = Color(1, 0, 0) * u + Color(0, 1.0, 0) * v + Color(0, 0, 1) * (1 - u - v);
                }

                _image(px, py) = finalColor;
            }
            else
                _image(px, py) = Renderer::BACKGROUND_COLOR;
        }
    }
}

void Renderer::post_process()
{
    Timer timer;

    if (_render_settings.enable_ssao)
    {
        float min = INFINITY;
        for (int i = 0; i < 1; i++)
        {
            timer.start();
            post_process_ssao();
            timer.stop();
            if (min > timer.elapsed())
                min = timer.elapsed();

            std::cout << "SSAO Time: " << timer.elapsed() << "ms" << std::endl;
        }

        std::cout << "min time: " << min << "ms" << std::endl;
    }
}

//void Renderer::post_process_ssao()
//{
//    int render_width, render_height;
//    get_render_width_height(_render_settings, render_width, render_height);
//
//    short int* ao_buffer = new short int[render_height * render_width];
//    std::memset(ao_buffer, 0, sizeof(short int) * render_height * render_width);
//
//    XorShiftGenerator rand_generator;
//#pragma omp parallel private(rand_generator)
//    {
//        rand_generator = XorShiftGenerator(omp_get_thread_num() * 24183 + rand());
//#pragma omp for
//        for (int y = 0; y < render_height; y++)
//        {
//            for (int x = 0; x < render_width; x++)
//            {
//                //No information here, there's no pixel to shade: background at this pixel on the image
//                if (_z_buffer(y, x) == INFINITY)
//                    continue;
//
//                float x_ndc = (float)x / render_width * 2 - 1;
//                float y_ndc = (float)y / render_height * 2 - 1;
//
//                float view_z = _z_buffer(y, x);
//                float view_ray_x = x_ndc * _scene._camera._aspect_ratio * std::tan(radians(_scene._camera._fov / 2));
//                float view_ray_y = y_ndc * std::tan(radians(_scene._camera._fov / 2));
//                Point camera_space_point = Point(view_ray_x * view_z, view_ray_y * view_z, -view_z);
//
//                Vector normal = normalize(_normal_buffer(y, x));
//
//                short int pixel_occlusion = 0;
//                for (int i = 0; i < _render_settings.ssao_sample_count; i++)
//                {
//                    float rand_x = rand_generator.get_rand_bilateral();
//                    float rand_y = rand_generator.get_rand_bilateral();
//                    float rand_z = rand_generator.get_rand_bilateral();
//
//                    Point random_sample = Point(normalize(Vector(rand_x, rand_y, rand_z)));
//                    random_sample = random_sample * (rand_generator.get_rand_lateral() + 0.0001);
//                    random_sample = random_sample * _render_settings.ssao_radius;
//                    random_sample = random_sample + camera_space_point;
//                    if (dot(random_sample - camera_space_point, normal) < 0)//The point is not in front of the normal
//                        random_sample = random_sample + 2 * (camera_space_point - random_sample);
//
//                    Point random_sample_ndc = _scene._camera._perspective_proj_mat(random_sample);
//                    int random_point_pixel_x = (int)((random_sample_ndc.x + 1) * 0.5 * render_width);
//                    int random_point_pixel_y = (int)((random_sample_ndc.y + 1) * 0.5 * render_height);
//
//                    random_point_pixel_x = std::min(std::max(0, random_point_pixel_x), render_width - 1);
//                    random_point_pixel_y = std::min(std::max(0, random_point_pixel_y), render_height - 1);
//
//                    float sample_geometry_depth = -_z_buffer(random_point_pixel_y, random_point_pixel_x);
//
//                    //Range check
//                    if (std::abs(sample_geometry_depth - camera_space_point.z) > _render_settings.ssao_radius)
//                        continue;
//                    if (random_sample.z < sample_geometry_depth)
//                        pixel_occlusion++;
//                }
//
//                ao_buffer[y * render_width + x] = pixel_occlusion;
//            }
//        }
//    }
//
//    //Blurring the AO
//    int blur_size = 7;
//    int half_blur_size = blur_size / 2;
//#pragma omp parallel for
//    for (int y = half_blur_size; y < render_height - half_blur_size; y++)
//    {
//        for (int x = half_blur_size; x < render_width - half_blur_size; x++)
//        {
//            if (_z_buffer(y, x) == INFINITY)//Background pixel, we're not blurring it
//                continue;
//
//            int sum = 0;
//            for (int offset_y = -half_blur_size; offset_y <= half_blur_size; offset_y++)
//                for (int offset_x = -half_blur_size; offset_x <= half_blur_size; offset_x++)
//                    sum += ao_buffer[(y + offset_y) * render_width + x + offset_x];
//
//            //Applying directly on the image
//            float color_multiplier = 1 - ((float)sum / (float)(blur_size * blur_size) / (float)_render_settings.ssao_sample_count * (float)_render_settings.ssao_amount);
//            _image(x, y) = _image(x, y) * Color(color_multiplier, color_multiplier, color_multiplier, 1.0f);
//        }
//    }
//
//    delete[] ao_buffer;
//}

void Renderer::post_process_ssao()
{
    int render_width, render_height;
    get_render_width_height(_render_settings, render_width, render_height);

    int* ao_buffer = new int[render_height * render_width];
    std::memset(ao_buffer, 0, sizeof(int) * render_height * render_width);

    __m256 render_height_vec = _mm256_set1_ps(render_height);
    __m256 render_width_vec = _mm256_set1_ps(render_width);

    __m256 ones = _mm256_set1_ps(1);
    __m256 twos = _mm256_set1_ps(2);
    __m256 fov_multiplier = _mm256_tan_ps(_mm256_mul_ps(_mm256_div_ps(_mm256_set1_ps(_scene._camera._fov / 2), _mm256_set1_ps(180)), _mm256_set1_ps(M_PI)));

    __m256_XorShiftGenerator rand_generator;
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

        XorShiftGenerator rand_generator_scalar = XorShiftGenerator(rand_generator._state.m256i_i32[0]);
#pragma omp for
        for (int y = 0; y < render_height; y++)
        {
            __m256 y_vec = _mm256_set1_ps(y);
            __m256 y_ndc = _mm256_div_ps(y_vec, render_height_vec);
            y_ndc = _mm256_mul_ps(y_ndc, twos);
            y_ndc = _mm256_sub_ps(y_ndc, ones);

            for (int x = 0; x < render_width; x += 8)
            {
                __m256 x_vec = _mm256_set1_ps(x);

                __m256 xs = _mm256_set_ps(7, 6, 5, 4, 3, 2, 1, 0);
                xs = _mm256_add_ps(xs, x_vec);

                __m256 x_ndc = _mm256_div_ps(xs, render_width_vec);
                x_ndc = _mm256_mul_ps(x_ndc, twos);
                x_ndc = _mm256_sub_ps(x_ndc, ones);

                __m256 view_ray_x = _mm256_mul_ps(x_ndc, _mm256_mul_ps(fov_multiplier, _mm256_set1_ps(_scene._camera._aspect_ratio)));
                __m256 view_ray_y = _mm256_mul_ps(y_ndc, fov_multiplier);

                __m256 view_z = _mm256_load_ps(_z_buffer.row(y) + x);
                __m256Point camera_space_point = __m256Point(_mm256_mul_ps(view_z, view_ray_x), _mm256_mul_ps(view_z, view_ray_y), _mm256_mul_ps(view_z, _mm256_set1_ps(-1)));

                __m256Vector normal = _mm256_normalize(__m256Vector(_normal_buffer.row(y) + x));

                __m256i pixel_occlusion = _mm256_setzero_si256();
                for (int i = 0; i < _render_settings.ssao_sample_count; i++)
                {
                    __m256 rand_x = rand_generator.get_rand_bilateral();
                    __m256 rand_y = rand_generator.get_rand_bilateral();
                    __m256 rand_z = rand_generator.get_rand_bilateral();

                    __m256Point random_sample = __m256Point(_mm256_normalize(__m256Vector(rand_x, rand_y, rand_z)));
                    random_sample = random_sample * _mm256_add_ps(rand_generator.get_rand_lateral(), _mm256_set1_ps(0.0001));
                    random_sample = random_sample * _mm256_set1_ps(_render_settings.ssao_radius);
                    random_sample = random_sample + camera_space_point;

                    //Flipping the random smaple if the dot product with the normal is negative, i.e., the sample
                    //is below the surface of the hemisphere and we're thus flipping the point back into the hemisphere
                    //IN FRONT of the normal
                    __m256Vector vec_dot = random_sample - camera_space_point;
                    __m256 dot_result = _mm256_dot_product(vec_dot, normal);
                    __m256 dot_mask = _mm256_and_ps(_mm256_cmp_ps(dot_result, _mm256_setzero_ps(), _CMP_LT_OQ), ones);
                    __m256Vector backflip_vec = 2 * (camera_space_point - random_sample);

                    ///+ 2 * (camera_space_point - random_sample) if dot < 0
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

                    __m256 infinity = _mm256_set1_ps(INFINITY);
                    __m256 infinity_mask = _mm256_cmp_ps(view_z, infinity, _CMP_NEQ_OQ);
                    occluded_mask = _mm256_and_ps(occluded_mask, infinity_mask);

                    pixel_occlusion = _mm256_add_epi32(pixel_occlusion, _mm256_cvtps_epi32(occluded_mask));
                }

                //TODO handle scalar leftover

                _mm256_store_si256((__m256i*) & ao_buffer[y * render_width + x], pixel_occlusion);
            }

            //TODO handle scalar leftover
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
            _image(x, y) = _image(x, y) * Color(color_multiplier, color_multiplier, color_multiplier, 1.0f);
        }
    }

    delete[] ao_buffer;
}
