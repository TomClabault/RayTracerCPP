#ifndef SCENE_H
#define SCENE_H

#include <vector>

#include "camera.h"
#include "light.h"
#include "triangle.h"
#include "mesh_io.h"

struct Scene
{
	Scene(Camera camera, std::vector<Triangle>& triangles, Materials _materials, PointLight point_light) 
		: _camera(camera), _point_light(point_light), _triangles(triangles), _materials(_materials) {}

	Camera _camera;
	PointLight _point_light;

	std::vector<Triangle> _triangles;
	Materials _materials;
};

#endif