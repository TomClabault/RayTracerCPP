#ifndef RAY_TRACER_H
#define RAY_TRACER_H

#include "image.h"
#include "scene.h"
#include "settings.h"

class RayTracer
{
public:
	static constexpr float EPSILON = 1.0e-4;
	static constexpr float SHADOW_INTENSITY = 0.5;
	static Material DEFAULT_MATERIAL;
	static Color AMBIENT_COLOR;

	RayTracer(unsigned int width, unsigned int height, Scene scene);

	Image* getImage();

	void trace();

private:
	/*
	 * @return Returns the diffuse color of the material given the intersection normal and the direction to the light source
	 */
	Color computeDiffuse(const Material& hitMaterial, const Vector& normal, const Vector& direction_to_light);

	Color computeSpecular(const Material& hitMaterial, const Vector& ray_direction, const Vector& inter_point, const Vector& normal, const Vector& direction_to_light);

	/*
	 * @return Returns true if the point is shadowed by another object, false otherwise.
	 */
	bool isShadowed(const Vector& inter_point, const Vector& light_position);

private:
	unsigned int _width, _height;

	Image _image;

	Scene _scene;
};

#endif