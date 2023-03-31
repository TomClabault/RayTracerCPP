#ifndef RAY_TRACER_H
#define RAY_TRACER_H

#include <array>

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

    void set_materials(Materials materials);

    void clear_z_buffer();
    void clear_normal_buffer();
    void clear_image();

    void change_camera_fov(float fov);
    void change_camera_aspect_ratio(float aspect_ratio);

    /**
     * @brief Used to change the render size of the renderer even after the renderer has been instantiated.
     * When the renderer has been instantiated, you should always use this function to change the render size
     * and not directly change the render_settings
     * @param width The new width
     * @param height The new height
     */
    void change_render_size(int width, int height);

	/*
	 * Ray traces only one triangle and returns its color given a ray
	 */
	Color trace_triangle(const Ray& ray, const Triangle& triangle) const;

	/*
	 * Renders the image using an hybrid rasterization / ray-tracing approach
	 */
	void raster_trace();
	/*
	 * Renders the image full ray tracing
	 */
	void ray_trace();

	/*
	 * Applies post-processing such as SSAO, FXAA, ...
	 */
	void post_process();

	/*
	 * Applies screen space ambient occlusion
	 */
	void post_process_ssao();

private:

    void init_buffers(int width, int height);

	/*
	 * @return Returns the diffuse color of the material given the intersection normal and the direction to the light source
	 */
	Color computeDiffuse(const Material& hitMaterial, const Vector& normal, const Vector& direction_to_light) const;

	Color computeSpecular(const Material& hitMaterial, const Vector& ray_direction, const Vector& normal, const Vector& direction_to_light) const;

	/*
	 * @return Returns true if the point is shadowed by another object, false otherwise.
	 */
	bool is_shadowed(const Point& inter_point, const Point& light_position) const;

	/*
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

	/*
	 * Clips a triangle in homogeneous clip space coordinates and outputs the clipped triangles in @param clipped_triangles
	 */
	int clip_triangle(std::array<Triangle4, 12>& to_clip_triangles, std::array<Triangle4, 12>& clipped_triangles) const;
	//int clip_triangle(const Triangle4& to_clip_triangle, std::array<Triangle4, 12>& clipped_triangles) const;

private:
    Buffer<float> _z_buffer;
    Buffer<Vector> _normal_buffer;//2D-Array of pointer to Vector.
	//The pointer to Vector trick allows us to store a normal for 8 bytes
	//(64 bit pointer) instead of 12 (3*4 floats)

    std::vector<Triangle> _triangles;
    Materials _materials;//Materials of the triangles
	 
	RenderSettings _render_settings;

	BVH _bvh;

	Image _image;
	Scene _scene;
};

#endif