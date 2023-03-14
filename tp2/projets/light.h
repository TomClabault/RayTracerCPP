#ifndef LIGHT_H
#define LIGHT_H

#include "vec.h"

struct PointLight
{
	PointLight(Vector position) : _position(position) {}

	Vector _position;
};

#endif