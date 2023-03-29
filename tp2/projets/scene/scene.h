#ifndef SCENE_H
#define SCENE_H

#include <vector>

#include "bvh.h"
#include "camera.h"
#include "light.h"
#include "triangle.h"
#include "mesh_io.h"

struct Scene
{
    Scene() {}
	Scene(Camera camera, Materials materials, PointLight point_light)
        : _camera(camera), _point_light(point_light) {}

	Camera _camera;

	PointLight _point_light;
};

#endif
