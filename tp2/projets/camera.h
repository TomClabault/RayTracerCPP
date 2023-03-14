#ifndef CAMERA_H
#define CAMERA_H

#include <vector>

#include "triangle.h"

struct Camera
{
	Camera(Vector position) : _position(position) {}

	Vector _position;
};

#endif