#include "triangle.h"

Triangle::Triangle() :  _a(Point(0, 0, 0)),
                        _b(Point(1, 0, 0)),
                        _c(Point(0, 1, 0)),
                        _normal(cross(_b - _a, _c - _a)),
                        _materialIndex(-1) {}

Triangle::Triangle(Point a, Point b, Point c, int material_index, const Point& tex_coords_u, const Point& tex_coords_v) :
    _a(a), _b(b), _c(c), _normal(cross(_b - _a, _c - _a)), _materialIndex(material_index), _tex_coords_u(tex_coords_u), _tex_coords_v(tex_coords_v) {};

Triangle::Triangle(const Triangle4& triangle, int material_index, const Point& tex_coords_u, const Point& tex_coords_v) :
    _materialIndex(material_index), _tex_coords_u(tex_coords_u), _tex_coords_v(tex_coords_v)
{
    float iaw, ibw, icw;
    iaw = 1.0f / triangle._a.w;
    ibw = 1.0f / triangle._b.w;
    icw = 1.0f / triangle._c.w;

    _a = Point(triangle._a.x * iaw, triangle._a.y * iaw, triangle._a.z * iaw);
    _b = Point(triangle._b.x * ibw, triangle._b.y * ibw, triangle._b.z * ibw);
    _c = Point(triangle._c.x * icw, triangle._c.y * icw, triangle._c.z * icw);
}

bool Triangle::intersect(const Ray& ray, HitInfo& hitInfo) const
{
    //Common variables to both algorithms
    float t, u, v;

    //TODO investigate muller trumbore black dots on very big 3d models
#if MOLLER_TRUMBORE
    Vector ab = _b - _a;
    Vector ac = _c - _a;
    Vector OA = ray._origin - _a;
    Vector minusDcrossOA = cross(-ray._direction, OA);

    float Mdet = dot(_normal, -ray._direction);
#if BACKFACE_CULLING
    if (Mdet <= 0)//If Mdet < 0, triangle back-facing. If == 0, ray parallel to triangle
        return false;
#else
    if (Mdet == 0)//If == 0, ray parallel to triangle
        return false;
#endif

    Mdet = 1 / Mdet;//Inverting the determinant once and for all

    //Cramer's rule
    u = dot(minusDcrossOA, ac) * Mdet;
    if (u < 0 || u > 1)
        return false;

    v = dot(minusDcrossOA, -ab) * Mdet;
    if (v < 0 || u + v > 1)
        return false;

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
    Point inter_point = ray._origin + ray._direction * t;
    if (!barycentric_coordinates(inter_point, u, v))
        return false;//We have an intersection with the plane of the triangle but the point isn't in the triangle
#endif

    //Common operations to both algorithms
    hitInfo.t = t;
    //Barycentric coordinates are: P = (1 - u - v)A + uB + vC
    hitInfo.u = u;
    hitInfo.v = v;
    hitInfo.tangent = get_tangent(ab, ac);
    hitInfo.mat_index = _materialIndex;
    hitInfo.normal_at_intersection = normalize(_normal);
    hitInfo.triangle = this;

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

bool Triangle::barycentric_coordinates(const Point& point, float& u, float& v) const
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

Vector Triangle::get_tangent(const Vector& ab, const Vector& ac) const
{
    float u1 = _tex_coords_u.x, v1 = _tex_coords_v.x;
    float u2 = _tex_coords_u.y, v2 = _tex_coords_v.y;
    float u3 = _tex_coords_u.z, v3 = _tex_coords_v.z;

    float deltaU1 = u2 - u1;
    float deltaV1 = v2 - v1;
    float deltaU2 = u3 - u1;
    float deltaV2 = v3 - v1;

    float f = 1.0f / (deltaU1 * deltaV2 - deltaU2 * deltaV1);

    Vector tangent;
    tangent.x = f * (deltaV2 * ab.x - deltaV1 * ac.x);
    tangent.y = f * (deltaV2 * ab.y - deltaV1 * ac.y);
    tangent.z = f * (deltaV2 * ab.z - deltaV1 * ac.z);

    return tangent;
}

void Triangle::interpolate_texcoords(float u, float v, float& out_tex_u, float& out_tex_v) const
{
    //Barycentric coordinates are: P = wA + uB + vC
    out_tex_u = (1 - u - v) * _tex_coords_u.x + u * _tex_coords_u.y + v * _tex_coords_u.z;
    out_tex_v = (1 - u - v) * _tex_coords_v.x + u * _tex_coords_v.y + v * _tex_coords_v.z;
}

Point Triangle::bbox_centroid() const
{
    return (min(_a, min(_b, _c)) + max(_a, max(_b, _c))) / 2;
}

bool Triangle::inside_outside_2D(const Point& point) const
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
    os << "Triangle[" << triangle._a << ", " << triangle._b << ", " << triangle._c << "], UVs: ";
    for (int i = 0; i < 3; i++)
        os << "(" << triangle._tex_coords_u(i) << ", " << triangle._tex_coords_v(i) << "), ";

    return os;
}

std::ostream& operator << (std::ostream& os, const Triangle4& triangle)
{
    os << "[" << triangle._a << ", " << triangle._b << ", " << triangle._c << "]";

    return os;
}

Triangle operator +(const Triangle& triangle, const Vector& vec)
{
    return Triangle(triangle._a + vec, triangle._b + vec, triangle._c + vec);
}

Point& Triangle::operator[] (int i)
{
    return *((&_a) + i);
}

const Point& Triangle::operator[] (int i) const
{
    return *((&_a) + i);
}

std::ostream& operator <<(std::ostream& os, const HitInfo& infos)
{
    os << "HitInfo[t="<< infos.t << ", u, v=(" << infos.u << ", " << infos.v << "), normal=" << infos.normal_at_intersection << "]";

    return os;
}
