#ifndef RAY_TRACER_H
#define RAY_TRACER_H

#include <QImage>

#include <array>
#include <mutex>
#include <omp.h>

#include "analyticShape.h"
#include "buffer.h"
#include "bvh.h"
#include "image.h"
#include "materials.h"
#include "rendererSettings.h"
#include "scene/scene.h"
#include "skybox.h"
#include "xorshift.h"

class Renderer
{
public:
	static constexpr float EPSILON = 1.0e-4f;
	static constexpr float SHADOW_INTENSITY = 0.5f;
    static Material DEFAULT_MATERIAL;
    static Material DEFAULT_PLANE_MATERIAL;
    static Material DEBUG_MATERIAL_1;
	static Color AMBIENT_COLOR;
	static Color BACKGROUND_COLOR;

    static std::vector<XorShiftGenerator> _xorshift_generators;

    /**
     * @return A diffuse material that has a nice pastel color
     */
    static Material get_random_diffuse_pastel_material();

    Renderer(Scene scene, std::vector<Triangle> triangles, RenderSettings render_settings);
    Renderer();

    void lock_image_mutex();
    void unlock_image_mutex();

    QImage* get_image();

	RenderSettings& render_settings();

    /**
     * @brief Computes the effective render height and width (accounting for SSAA for example)
     *  based on the given render settings and stores the output in render_width and render_height
     * @param settings The render settings
     * @param[out] render_width The effective render width
     * @param[out] render_height The effective render height
     */
    void get_render_width_height(const RenderSettings& settings, int& render_width, int& render_height);

    void set_triangles(const std::vector<Triangle>& triangles);

    void add_analytic_shape(const AnalyticShapesTypes& shape);

    Materials& get_materials();
    void set_materials(Materials materials);

    void prepare_ssao_buffers();
    void destroy_ssao_buffers();

    void clear_z_buffer();
    void clear_normal_buffer();
    void clear_image();
    void clear_geometry();

    void change_camera_fov(float fov);
    void change_camera_aspect_ratio(float aspect_ratio);

    void set_light_position(const Point& position);

    void set_ao_map(const Image& ao_map);
    void set_diffuse_map(const Image& diffuse_map);
    void set_normal_map(const Image& normal_map);
    void set_displacement_map(const Image& displacement_map);
    void set_roughness_map(const Image& displacement_map);

    void set_skysphere(const Image& skybox);
    void set_skybox(const Skybox& skybox);

    void clear_ao_map();
    void clear_diffuse_map();
    void clear_normal_map();
    void clear_displacement_map();
    void clear_roughness_map();

    Color sample_texture(const Image& texture, float tex_coord_u, float tex_coord_v) const;
    
    void reset_previous_transform();

    void set_object_transform(const Transform& object_transform);
    void set_camera_transform(const Transform& camera_transform);
    void apply_transformation_to_camera(const Transform& additional_transformation);

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
    Color trace_triangle(const Ray& ray, const Triangle& triangle, int current_recursion_depth) const;

    /**
     * @brief Renders the image using an hybrid rasterization / ray-tracing approach
     */
	void raster_trace();

    /**
     * @brief trace_ray Traces a ray and computes the color returned by that ray
     * @param ray The ray
     * @param hit_info The hit information. Only relevant if an intersection was found
     * @param current_recursion_depth Used to keep track of the current depth we're at
     * and compare it against the max_recursion_depth parameter of RenderSettings to
     * avoid infinite recursion
     * @param [out] intersection_found Whether or not an intersection was found
     * @return The color of the ray.
     */
    Color trace_ray(const Ray& ray, HitInfo& hit_info, int current_recursion_depth, bool& intersection_found) const;

    /**
     * @brief Renders the image full ray tracing
     */
	void ray_trace();

	/*
	 * Applies post-processing such as SSAO, FXAA, ...
	 */
	void post_process();

    /**
     * @brief Takes the current frame buffer and downscales it according
     * to the specified SSAA factor of the render settings. The frame buffer
     * is then recreated with the proper size (and the proper anti-aliased look).
     * The old framebuffer (that is 'SSAA_factor' times larger than the final image)
     * is lost.
     */
    void apply_ssaa();

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

    Color compute_reflection(const Ray& ray, const Point& inter_point, const HitInfo& hit_info, int current_recursion_depth) const;

    /**
     * @return Returns true if the point is shadowed by another object
     * according to the given light position, false otherwise.
	 */
    bool is_shadowed(const Point& inter_point, const Vector& normal_at_intersection, const Point& light_position) const;

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
     * @brief This function basically interpolates the tex coords at the intersection
     * point between the three vertices of the intersected triangle and stores the
     * interpolated u and v in \param tex_coord_u and \param tex_coord_v. If the intersected
     * shape isn't a triangle, \param tex_coord_u and \param tex_coord_v are set to
     * \param u and \param v respectively
     * @param triangle The intersected triangle. nullptr if an object other than a triangle was intersected
     * @param u Input u coordinate to interpolate
     * @param v Input v coordinate to interpolate
     * @param [out] tex_coord_u The computed u texcoord
     * @param [out] tex_coord_v The computed v texcoord
     */
    void get_tex_coords(const Triangle* triangle, float u, float v, float& tex_coord_u, float& tex_coord_v) const;

    /**
     * @brief Computes the ambient occlusion at the intersection point given
     * by the hit_info struct and returns the occlusion factor
     * @param hit_info The HitInfo struct
     * @param u The u (from UV) coordinate to use for the texture sampling. Useful
     * if using UV coordinates from displacement mapping is needed
     * @param v The v (from UV) coordinate to use for the texture sampling. Useful
     * if using UV coordinates from displacement mapping is needed
     * @return The ambient occlusion factor
     */
    float ao_mapping(const HitInfo& hit_info, float u, float v) const;

    /**
     * @brief Computes the color from the diffuse map contained in the
     * renderer using the given HitInfo
     * @param hit_info The HitInfo struct
     * @param u The u (from UV) coordinate to use for the texture sampling. Useful
     * if using UV coordinates from displacement mapping is needed
     * @param v The v (from UV) coordinate to use for the texture sampling. Useful
     * if using UV coordinates from displacement mapping is needed
     * @return The appropriate diffuse color
     */
    Color diffuse_mapping(const HitInfo& hit_info, float u, float v) const;

    /**
     * @brief Computes and returns the perturbed normal using the hit_info struct
     * passed
     * @param hit_info The structure containing the information about the intersection
     * we just found
     * @param u The u (from UV) coordinate to use for the texture sampling. Useful
     * if using UV coordinates from displacement mapping is needed
     * @param v The v (from UV) coordinate to use for the texture sampling. Useful
     * if using UV coordinates from displacement mapping is needed
     * @return The perturbed normal
     */
    Vector normal_mapping(const HitInfo& hit_info, float u, float v) const;

    void parallax_mapping(const Triangle* triangle, float u, float v, const Point& original_inter_point, const Vector& view_dir, float& new_u, float& new_v) const;
    void steep_parallax_mapping(const Triangle* triangle, float u, float v, const Point& original_inter_point, const Vector& view_dir, float& new_u, float& new_v) const;
    void parallax_occlusion_mapping(const Triangle* triangle, float u, float v, const Point& original_inter_point, const Vector& view_dir, float& new_u, float& new_v) const;

    /**
     * @brief Computes the color of the point intersection of a ray and the scene
     * @param ray The ray that intersected the scene
     * @param hit_info The information about the intersection that occured
     * @param current_recursion_depth Used to keep track of the current depth we're at
     * and compare it against the max_recursion_depth parameter of RenderSettings to
     * avoid infinite recursion
     * @return The color at the point of intersection. The color depends on the
     * shading method set in the renderer settings
     */
    Color shade_ray_inter_point(const Ray& ray, HitInfo& hit_info, int current_recursion_depth) const;

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

    std::vector<AnalyticShapesTypes> _analytic_shapes;
	 
	RenderSettings _render_settings;

	BVH _bvh;

    //Mutex used when the image buffer is being harsly modifier by the renderer
    //such as a memory move at the end of the SSAA downscale method for example
    //to avoid other threads (mainly the display) to use the image while it's in
    //an invalid memory state
    std::mutex _image_mutex;
    QImage _image;

    //TODO mettre ces images dans les Materiaux
    Image _ao_map;
    Image _diffuse_map;
    Image _normal_map;
    Image _displacement_map;
    Image _roughness_map;

    Image _skysphere;
    Skybox _skybox;

	Scene _scene;
};

#endif
