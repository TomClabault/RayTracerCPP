#include "analyticShape.h"

#define _USE_MATH_DEFINES

#include "math.h"

Sphere::Sphere(const Point& center, float radius, int mat_index) : _center(center), _radius(radius), _radius2(radius * radius), _mat_index(mat_index) { }

bool Sphere::intersect(const Ray& ray, HitInfo& hit_info, bool compute_uv) const
{
    compute_uv = true;

    Vector L = ray._origin - _center;
    constexpr float a = 1;//dot(ray._direction, ray._direction) = 1 because direction is normalized
    float b = 2 * dot(ray._direction, L);
    float c = dot(L, L) - _radius2;

    float delta = b * b - 4 * a * c;
    if (delta < 0)
        return false;
    else
    {
        constexpr float a2 = 2 * a;

        if (delta == 0.0)
            hit_info.t = -b / a2;
        else
        {
            float sqrt_delta = std::sqrt(delta);

            float t1 = (-b - sqrt_delta) / a2;
            float t2 = (-b + sqrt_delta) / a2;

            if (t1 < t2)
            {
                hit_info.t = t1;
                if (hit_info.t < 0)
                    hit_info.t = t2;
            }
        }

        if (hit_info.t < 0)
            return false;

        hit_info.normal_at_intersection = normalize((ray._origin + ray._direction * hit_info.t) - _center);

        if (compute_uv)
        {
            hit_info.u = 0.5 + std::atan2(-hit_info.normal_at_intersection.z, -hit_info.normal_at_intersection.x) / (2 * M_PI);
            hit_info.v = 0.5 + std::asin(-hit_info.normal_at_intersection.y) / M_PI;

            //NOTE (Tom): This creates singularities at the poles!
            hit_info.tangent = cross(Vector(0, 0, 1), hit_info.normal_at_intersection);
        }

        hit_info.mat_index = _mat_index;

        return true;
    }
}

Plane::Plane(const Point& point, const Vector& normal, int mat_index) : _point(point), _normal(normal), _mat_index(mat_index) {}

bool Plane::intersect(const Ray& ray, HitInfo& hit_info) const
{
    float t = dot(_point - ray._origin, _normal) / dot(ray._direction, _normal);

    if (t < 0)
        return false;

    hit_info.t = t;
    hit_info.mat_index = _mat_index;
    hit_info.normal_at_intersection = _normal;

    return true;
}

std::ostream& operator << (std::ostream& os, const Sphere& sphere)
{
    os << "Sphere[" << sphere._center << ", r=" << sphere._radius << "]";

    return os;
}

std::ostream& operator << (std::ostream& os, const Plane& plane)
{
    os << "Plane[" << plane._point << ", " << plane._normal << "]";

    return os;
}
