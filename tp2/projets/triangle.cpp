#include "triangle.h"

Triangle::Triangle() :  _a(Vector(0, 0, 0)),
                        _b(Vector(1, 0, 0)),
                        _c(Vector(0, 1, 0)),
                        _normal(cross(_b - _a, _c - _a)),
                        _materialIndex(-1) {}

Triangle::Triangle(Vector a, Vector b, Vector c) : _a(a), _b(b), _c(c), _normal(cross(_b - _a, _c - _a)), _materialIndex(-1) {};

bool Triangle::intersect(const Ray& ray, HitInfo& hitInfo) const
{
    //Common variables to both algorithms
    float t, u, v;

#if MOLLER_TRUMBORE
    Vector ab = _b - _a;
    Vector ac = _c - _a;
    Vector OA = ray._origin - _a;
    Vector minusDcrossOA = cross(-ray._direction, OA);

    float Mdet = dot(_normal, -ray._direction);
    if (Mdet == 0)//If Mdet > 0, triangle back-facing. If == 0, ray parallel to triangle
        return false;

    Mdet = 1 / Mdet;//Inverting the determinant once and for all

    //Cramer's rule
    u = dot(minusDcrossOA, ac) * Mdet;
    if (u < 0 || u > 1)
        return false;
    hitInfo.u = u;

    v = dot(minusDcrossOA, -ab) * Mdet;
    if (v < 0 || u + v > 1)
        return false;
    hitInfo.v = v;

    //Intersection point in the triangle at this point, computing t
    t = dot(_normal, OA) * Mdet;
    if (t < 0)
        return false;

#else
    //Is there an intersection with the plane of the triangle ?
    float denom = dot(_normal, ray._direction);
    if (denom == 0)
        return false;

    t = dot(_normal, _a - ray._origin) / denom;
    //We have an intersection with the plane of the 
    //triangle but it's behind the origin of the ray
    if (t < 0)
        return false;

    //Now testing if the point is in the triangle
    Vector inter_point = ray._origin + ray._direction * t;
    if (!barycentric_coordinates(inter_point, u, v))
        return false;//We have an intersection with the plane of the triangle but the point isn't in the triangle
#endif

    //Common operations to both algorithms
    hitInfo.t = t;
    hitInfo.u = u;
    hitInfo.v = v;
    hitInfo.mat_index = _materialIndex;
    hitInfo.normal_at_intersection = _normal;

    return true;
}

bool Triangle::intersect(const Ray& ray, float& t, float& u, float& v) const
{
    HitInfo hitInfo;

    bool returned = intersect(ray, hitInfo);
    t = hitInfo.t;
    u = hitInfo.u;
    v = hitInfo.v;

    return returned;
}

bool Triangle::barycentric_coordinates(const Vector& point, float& u, float& v) const
{
    Vector ab = _b - _a;
    Vector ac = _c - _a;

    float triangleArea = length(cross(ab, ac));

    Vector ca = _a - _c;
    Vector cp = point - _c;
    Vector crossVec = cross(ca, cp);
    if (dot(_normal, crossVec) < 0)
        return false;
    u = length(crossVec) / triangleArea;

    Vector ap = point - _a;
    crossVec = cross(ab, ap);
    if (dot(_normal, crossVec) < 0)
        return false;
    v = length(crossVec) / triangleArea;

    Vector bc = _c - _b;
    Vector bp = point - _b;
    crossVec = cross(bc, bp);
    if (dot(_normal, crossVec) < 0)
        return false;

    return true;
}

bool Triangle::inside_outside_2D(const Vector& point) const
{
    float abSide = (_b.x - _a.x) * (point.y - _a.y) - (_b.y - _a.y) * (point.x - _a.x);
    if (abSide < 0)
        return false;

    float bcSide = (_c.x - _b.x) * (point.y - _b.y) - (_c.y - _b.y) * (point.x - _b.x);
    if (bcSide < 0)
        return false;

    float caSide = (_a.x - _c.x) * (point.y - _c.y) - (_a.y - _c.y) * (point.x - _c.x);
    if (caSide < 0)
        return false;

    return true;
}

std::ostream& operator << (std::ostream& os, const Triangle& triangle)
{
    os << "[" << triangle._a << ", " << triangle._b << ", " << triangle._c << "]";

    return os;
}

std::ostream& operator <<(std::ostream& os, const HitInfo& infos)
{
    os << "HitInfo[t="<< infos.t << ", u, v=(" << infos.u << ", " << infos.v << "), normal=" << infos.normal_at_intersection << "]";

    return os;
}