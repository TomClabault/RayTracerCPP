#ifndef RAY_TRACER_H
#define RAY_TRACER_H

#include <array>

#include "analyticShape.h"
#include "buffer.h"
#include "image.h"
#include "rendererSettings.h"
#include "scene/scene.h"

class Renderer
{
public:
	static constexpr float EPSILON = 1.0e-4f;
	static constexpr float SHADOW_INTENSITY = 0.5f;
    static Material DEFAULT_MATERIAL;
    static Material DEBUG_MATERIAL_1;
	static Color AMBIENT_COLOR;
	static Color BACKGROUND_COLOR;

    Renderer(Scene scene, std::vector<Triangle> triangles, RenderSettings render_settings);
    Renderer();

	Image* getImage();

	RenderSettings& render_settings();

    /**
     * @brief Computes the effective render height and width (accounting for SSAA for example)
     *  based on the given render settings and stores the output in render_width and render_height
     * @param settings The render settings
     * @param[out] render_width The effective render width
     * @param[out] render_height The effective render height
     */
    void get_render_width_height(const RenderSettings& settings, int& render_width, int& render_height);

    void set_triangles(std::vector<Triangle> triangles);//TODO ne pas faire de copie. mais move?

    Materials& get_materials();
    void set_materials(Materials materials);

    void prepare_ssao_buffers();
    void destroy_ssao_buffers();

    void clear_z_buffer();
    void clear_normal_buffer();
    void clear_image();

    void change_camera_fov(float fov);
    void change_camera_aspect_ratio(float aspect_ratio);

    void set_light_position(const Point& position);

    void set_ao_map(const Image& ao_map);
    void set_diffuse_map(const Image& diffuse_map);
    void set_skybox(const Image& skybox);

    void clear_ao_map();
    void clear_diffuse_map();

    Color sample_texture(const Image& texture, float& tex_coord_u, float& tex_coord_v) const;
    
    void reset_previous_transform();

    void set_object_transform(const Transform& object_transform);
    void set_camera_transform(const Transform& camera_transform);

    /**
     * @brief Builds the BVH using the present triangles and the BVH
     * settings held in the RenderSettings
     */
    void reconstruct_bvh_new();

    /**
     * @brief Destroys the BVH
     */
    void destroy_bvh();

    /**
     * @brief Used to change the render size of the renderer even after the renderer has been instantiated.
     * When the renderer has been instantiated, you should always use this function to change the render size
     * and not directly change the render_settings
     * @param width The new width
     * @param height The new height
     */
    void change_render_size(int width, int height);

    /**
     * @brief Ray traces only one triangle and returns its color given a ray
     * @param ray The ray
     * @param triangle The triangle
     * @return The color of the intersection between the ray and the triangle
     * if it exists
     */
	Color trace_triangle(const Ray& ray, const Triangle& triangle) const;

    /**
     * @brief Renders the image using an hybrid rasterization / ray-tracing approach
     */
	void raster_trace();

    /**
     * @brief trace_ray Traces a ray and computes the color returned by that ray
     * @param ray The ray
     * @param hit_info The hit information. Only relevant if an intersection was found
     * @param [out] intersection_found Whether or not an intersection was found
     * @return The color of the ray.
     */
    Color trace_ray(const Ray& ray, HitInfo& hit_info, bool& intersection_found) const;

    /**
     * @brief Renders the image full ray tracing
     */
	void ray_trace();

	/*
	 * Applies post-processing such as SSAO, FXAA, ...
	 */
	void post_process();

    /**
     * @brief Applies screen space ambient occlusion
     */
    void post_process_ssao_SIMD();
    void post_process_ssao_scalar();

private:

    void init_buffers(int width, int height);

    /**
	 * @return Returns the diffuse color of the material given the intersection normal and the direction to the light source
	 */
    Color compute_diffuse(const Material& hitMaterial, const Vector& normal, const Vector& direction_to_light) const;
    
    Color compute_specular(const Material& hitMaterial, const Vector& ray_direction, const Vector& normal, const Vector& direction_to_light) const;

    Color compute_reflection(const Ray& ray, const HitInfo& hit_info) const;

    /**
     * @return Returns true if the point is shadowed by another object
     * according to the given light position, false otherwise.
	 */
    bool is_shadowed(const Point& inter_point, const Point& light_position) const;

    /**
     * @brief Returns the color based on the given normalized normal vector
     * @param normalized_normal Normalized normal
     * @return (Color(normalized_normal.x, normalized_normal.y, normalized_normal.z) + Color(1.0f, 1.0f, 1.0f)) * 0.5
     */
    Color shade_abs_normals(const Vector& normalized_normal) const;

    /**
     * @brief Returns the color based on the given normalized normal vector
     * @param normalized_normal Normalized normal
     * @return (Color(normalized_normal.x, normalized_normal.y, normalized_normal.z) + Color(1.0f, 1.0f, 1.0f)) * 0.5
     */
    Color shade_pastel_normals(const Vector& normalized_normal) const;

    /**
     * @brief Returns the color based on the given barycentric coordinates
     * @param u U barycentric coordinate
     * @param v V barycentric coordinate
     * @return u * Red + v * Green + (1 - u - v) * w
     */
    Color shade_barycentric_coordinates(float u, float v) const;

    /**
     * @brief Shades a given intersection point (contained in hit_info) with a technique
     * that helps visualizing ambient occlusion
     * @param triangle The intersected triangle
     * @param u The u coordinate on the triangle at the intersection point
     * @param v The u coordinate on the triangle at the intersection point
     * @return The color at the shaded point
     */
    Color shade_visualize_ao(const Triangle& triangle, float u, float v) const;

    /**
     * @brief Computes the color of the point intersection of a ray and the scene
     * @param ray The ray that intersected the scene
     * @param hit_info The information about the intersection that occured
     * @return The color at the point of intersection. The color depends on the
     * shading method set in the renderer settings
     */
    Color shade_ray_inter_point(const Ray& ray, const HitInfo& hit_info) const;

    /**
	 * Clips triangles given in @to_clip against the plane defined by the given @plane_index and @plane_sign and
	 * stores the result in @out_clipped
	 *
	 * @template plane_index 0/1/2 for clipping against the x/y/z plane respectively
	 * @template plane_sign 1 or -1 to clip against x or -x (for example)
	 * 
	 * @param to_clip triangles that needs to be clipped against the plane
	 * @param nb_triangles how many triangles are there to clip in the @to_clip array starting
	 * from index 0
	 * @param out_clipped clipped triangles
	 *
	 * @return Returns the number of triangles after the clipping process
	 */
	template<int plane_index, int plane_sign>
	int clip_triangles_to_plane(std::array<Triangle4, 12>& to_clip, int nb_triangles, std::array<Triangle4, 12>& out_clipped) const;

    /**
     * @brief Clips a triangle in homogeneous clip space coordinates and outputs the clipped triangles in @param clipped_triangles
	 */
    int clip_triangle(std::array<Triangle4, 12>& to_clip_triangles, std::array<Triangle4, 12>& clipped_triangles) const;

    /**
     * @brief Transforms only the z coordinate of the given vertex with the given matrix
     * @param matrix The matrix
     * @param vertex The vertex whose z coordinate is to be transformed
     * @return The transformed z coordinate of the given vertex
     */
    float matrix_transform_z(const Transform& matrix, const Point& vertex);

public:
    void add_sphere(const Sphere& sphere) { spheres.push_back(sphere); }

private:
    Buffer<float> _z_buffer;
    Buffer<Vector> _normal_buffer;//2D-Array of pointer to Vector.
	//The pointer to Vector trick allows us to store a normal for 8 bytes
	//(64 bit pointer) instead of 12 (3*4 floats)

    std::vector<Triangle> _triangles;
    //Last transform used to transform the triangles. It is used to avoid
    //"stacking" transforms on top of each other by inverting the previous
    //transformation that was applied
    Transform _previous_object_transform = Identity();
    Materials _materials;//Materials

    std::vector<Sphere> spheres;
	 
	RenderSettings _render_settings;

	BVH _bvh;

	Image _image;
    Image _ao_map;
    Image _diffuse_map;
    Image _skybox;

	Scene _scene;
};

#endif
