#include "analyticShape.h"

#define _USE_MATH_DEFINES

#include "math.h"

Sphere::Sphere(const Point& center, float radius, int mat_index) : _center(center), _radius(radius), _mat_index(mat_index) { }

bool Sphere::intersect(const Ray& ray, HitInfo& hit_info)
{
    Vector origMinus2C = ray._origin - 2 * _center;

    float a = dot(ray._direction, ray._direction);
    float b  = dot(ray._direction, origMinus2C);
    float c = dot(Vector(ray._origin), origMinus2C) + dot(Vector(_center), Vector(_center)) - _radius * _radius;

    float delta = b * b - 4 * a * c;
    if (delta < 0)
        return false;
    else
    {
        hit_info.mat_index = _mat_index;

        if (delta == 0.0)
            hit_info.t = -b / (2 * a);
        else
            hit_info.t = std::min((-b - std::sqrt(delta)) / (2 * a), (-b + std::sqrt(delta)) / (2 * a));

        if (hit_info.t < 0)
            return false;

        hit_info.normal_at_intersection = (ray._origin + ray._direction * hit_info.t) - _center;

        hit_info.u = 0.5 + std::atan2(-hit_info.normal_at_intersection.z, -hit_info.normal_at_intersection.x) / (2 * M_PI);
        hit_info.v = 0.5 + std::asin(-hit_info.normal_at_intersection.y) / M_PI;

        return true;
    }
}
