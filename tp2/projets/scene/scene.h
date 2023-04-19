#ifndef SCENE_H
#define SCENE_H

#include <vector>

#include "camera.h"
#include "light.h"

struct Scene
{
    Scene() {}
    Scene(Camera camera, PointLight point_light)
        : _camera(camera), _point_light(point_light) {}

	Camera _camera;

	PointLight _point_light;
};

#endif
