#ifndef RAY_TRACER_H
#define RAY_TRACER_H

#include "image.h"
#include "scene.h"
#include "settings.h"

class RayTracer
{
public:
	RayTracer(unsigned int width, unsigned int height, Scene scene);

	Image* getImage();

	void trace();

private:
	unsigned int _width, _height;

	Image _image;

	Scene _scene;
};

#endif