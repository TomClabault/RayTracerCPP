#ifndef RAY_H
#define RAY_H

#include "vec.h"

class Ray
{
public:
    Ray(Vector origin, Vector direction);

    friend std::ostream& operator << (std::ostream& os, const Ray& ray);

public:
    Vector _origin, _direction;
};

#endif