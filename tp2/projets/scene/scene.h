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
	Scene(Camera camera, Materials materials, PointLight point_light)
		: _camera(camera), _point_light(point_light), _materials(materials) {}

	Camera _camera;

	PointLight _point_light;

	Materials _materials;//Materials of the triangles
};

#endif