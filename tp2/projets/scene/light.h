#ifndef LIGHT_H
#define LIGHT_H

#include "vec.h"

struct PointLight
{
    PointLight() : _position(Point(3, 3, 2)) {}
	PointLight(Point position) : _position(position) {}

	Point _position;
};

#endif
