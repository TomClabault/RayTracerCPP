#ifndef LIGHT_H
#define LIGHT_H

#include "vec.h"

struct PointLight
{
	PointLight(Point position) : _position(position) {}

	Point _position;
};

#endif