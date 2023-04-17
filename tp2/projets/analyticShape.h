#ifndef ANALYTIC_SHAPE_H
#define ANALYTIC_SHAPE_H

#include "hitInfo.h"
#include "ray.h"

template <typename T>
class AnalyticShape
{
    bool intersect(const Ray& ray, HitInfo& hit_info) const
    {
        static_cast<T>(*this).intersect(ray, hit_info);
    }
};

class Sphere : AnalyticShape<Sphere>
{
public:
    Sphere(const Point& center, float radius, int mat_index = 0);

    bool intersect(const Ray& ray, HitInfo& hit_info, bool compute_uv = false) const;

private:
    Point _center;

    float _radius;
    float _radius2;

    int _mat_index;
};

class Plane : AnalyticShape<Plane>
{
public:
    Plane(const Point& point, const Vector& normal, int mat_index = 0);

    bool intersect(const Ray& ray, HitInfo& hit_info) const;

private:
    Point _point;

    Vector _normal;

    int _mat_index;
};

#endif
