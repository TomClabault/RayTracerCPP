#ifndef RAY_TRACER_H
#define RAY_TRACER_H

#include "image.h"
#include "scene.h"
#include "settings.h"

class Renderer
{
public:
	static constexpr float EPSILON = 1.0e-4f;
	static constexpr float SHADOW_INTENSITY = 0.5f;
	static Material DEFAULT_MATERIAL;
	static Color AMBIENT_COLOR;

	Renderer(int width, int height, Scene scene);

	Image* getImage();

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