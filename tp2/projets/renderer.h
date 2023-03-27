#ifndef RAY_TRACER_H
#define RAY_TRACER_H

#include <array>

#include "image.h"
#include "scene/scene.h"

struct RenderSettings
{
	RenderSettings() {}
	RenderSettings(int width, int height) : image_width(width), image_height(height) {}

	static RenderSettings basic_settings(int width, int height, bool hybrid_raster_trace = true);
	static RenderSettings ssaa_settings(int width, int height, int ssaa_factor, bool hybrid_raster_trace = true, bool compute_shadows = false);

	int image_width = 1024;
	int image_height = 1024;

	bool enable_ssaa = false;
	//Super sampling factor. How many times larger will the image be rendered
	int ssaa_factor = 2;

	//Enable triangle clipping when rasterizing. 
	//This can be safely disabled to save performance if your objects fit in
	//the view frustum
	bool enable_clipping = true;

	//Whether or not to use rasterization to first determine the visibility of the 
	//triangles and then ray tracing for the rest of computations (shadows, reflections, ...)
	//0 for full ray-tracing
	//1 for rasterization/ray-tracing
	bool hybrid_rasterization_tracing = true;

	//1 to use shading, false to use 'color_normal_or_barycentric' for the shading. 
	//If this is true, 'color_normal_or_barycentric' is ignored
	bool use_shading = true;
	//true to compute shadows, false not to
	bool compute_shadows = true;

	//true to color the triangles with the normal
	//false to color the triangles with the barycentric coordinates
	//This parameter is only used if the SHADING define is set to false
	bool color_normal_or_barycentric = true;

	//Whether or not to use a BVH to intersect the scene
	bool enable_bvh = true;
	//Maximum depth of the BVH tree
	int bvh_max_depth = 13;
	//Maximum number of objects per leaf of the BVH tree if the maximum recursion depth
	//defined by bvh_max_depth hasn't been reached
	int bvh_leaf_object_count = 60;

	friend std::ostream& operator << (std::ostream& os, const RenderSettings& settings);
};

class Renderer
{
public:
	static constexpr float EPSILON = 1.0e-4f;
	static constexpr float SHADOW_INTENSITY = 0.5f;
	static Material DEFAULT_MATERIAL;
	static Color AMBIENT_COLOR;
	static Color BACKGROUND_COLOR;

	Renderer(Scene scene, std::vector<Triangle>& triangles, RenderSettings render_settings);
	~Renderer();

	Image* getImage();
	RenderSettings& render_settings();

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
	
	BVH _bvh;

private:
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
	//Width and height that account for the SSAA super sampling if enabled.
	//These values should always be used over _render_settings.image_width and
	//_render_settings.image_height when redering. _render_settings.image_width and
	//height are used to keep track of the actual resolution wanted "on screen", they
	//do not reflect the render resolution
	int _render_width, _render_height;

	float** _z_buffer;

	std::vector<Triangle>& _triangles;
	 
	RenderSettings _render_settings;

	Image _image;
	Scene _scene;
};

#endif