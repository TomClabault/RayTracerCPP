#ifndef RAY_TRACER_H
#define RAY_TRACER_H

#include <array>

#include "image.h"
#include "scene.h"
#include "settings.h"

struct ClippingPlane
{
	Vector _normal;

	float _d;
};

class Renderer
{
public:
	static constexpr float EPSILON = 1.0e-4f;
	static constexpr float SHADOW_INTENSITY = 0.5f;
	static Material DEFAULT_MATERIAL;
	static Color AMBIENT_COLOR;
	static std::array<ClippingPlane, 5> CLIPPING_PLANES;

	Renderer(int width, int height, Scene scene);
	~Renderer();

	Image* getImage();

	/*
	 * Clips the non-visible triangles of the scene
	 * 
	 * @returns Nothing but directly modifies the triangles of the scene
	 */
	void clip();

	/*
	 * Clips the non-visible triangles of the given triangle vector
	 *
	 * @returns The visible triangles
	 */
	std::vector<Triangle> clip(const std::vector<Triangle>& triangles);

	/*
	 * Ray traces only one triangle and returns its color given a ray
	 */
	Color traceTriangle(const Ray& ray, const Triangle& triangle) const;

	/*
	 * Renders the image using an hybrid rasterization / ray-tracing approach
	 */
	void rasterTrace();
	/*
	 * Renders the image full ray tracing
	 */
	void rayTrace();

private:
	/*
	 * When a triangle isn't completely inside or outside the volume defined by a plane, it must be clipped and produce new triangles. This functions does that.
	 * a_inside, b_inside and c_inside are boolean defining whether or not the vertex a, b and c respectively are inside or outside the volume defined by the plane
	 */
	std::vector<Triangle> clip_triangle_against_plane(const Triangle& triangle, const ClippingPlane& plane, bool a_inside, bool b_inside, bool c_inside);

	/*
	 * @return Returns the diffuse color of the material given the intersection normal and the direction to the light source
	 */
	Color computeDiffuse(const Material& hitMaterial, const Vector& normal, const Vector& direction_to_light) const;

	Color computeSpecular(const Material& hitMaterial, const Vector& ray_direction, const Vector& normal, const Vector& direction_to_light) const;

	/*
	 * @return Returns true if the point is shadowed by another object, false otherwise.
	 */
	bool isShadowed(const Vector& inter_point, const Vector& light_position) const;

private:
	int _width, _height;

	Image _image;

	float** _z_buffer;

	Scene _scene;
};

#endif