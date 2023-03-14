#include "ray.h"

Ray::Ray(Vector origin, Vector direction) : _origin(origin), _direction(direction) {}

std::ostream& operator << (std::ostream& os, const Ray& ray)
{
    os << "Ray[" << ray._origin << "->" << ray._direction << "]";

    return os;
}
