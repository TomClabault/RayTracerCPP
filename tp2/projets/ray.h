#ifndef RAY_H
#define RAY_H

#include "vec.h"

class Ray
{
public:
    Ray(Point origin, Vector direction);

    friend std::ostream& operator << (std::ostream& os, const Ray& ray);

public:
    Point _origin;

    Vector _direction;
};

#endif