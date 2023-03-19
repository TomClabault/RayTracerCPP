#ifndef TRIANGLE_H
#define TRIANGLE_H

#include "ray.h"
#include "settings.h"

#include "vec.h"

struct HitInfo
{
    //Intersection distance to the triangle
    float t = -1;

    //Barycentric coordinates of the intersection point in the triangle
    float u = 1.0, v = 0.0;
    int mat_index = -1;//Index of the material of the intersected triangle. 
    //This index can be used to retrieve the material in the Materials array in the Scene

    //Normal at the intersection point. NON NORMALIZED!!
    Vector normal_at_intersection;

    friend std::ostream& operator <<(std::ostream& os, const HitInfo& infos);
};

//Triangle with homogeneous coordinates vertices
struct Triangle4
{
    Triangle4() {}
    Triangle4(vec4 a, vec4 b, vec4 c) : _a(a), _b(b), _c(c) {}

    vec4 _a, _b, _c;
};

class Triangle
{
public:
    constexpr static double EPSILON = 1.0e-4;

    Triangle();
    Triangle(Vector a, Vector b, Vector c);

    /*
     * Converts a triangle 4 in homogeneous coordinates to a cartesian triangle by dividing each verte by their homogeneous w component
     */
    Triangle(Triangle4 triangle);

    bool intersect(const Ray& ray, HitInfo& hitInfo) const;
    bool intersect(const Ray& ray, float& t, float& u, float& v) const;

    /*
     * Inside/outside test considering the triangle's vertices to all have equal z coordinates.
     * This test essentially ignores the z coordinates of the triangle's vertices
     */
    bool inside_outside_2D(const Vector& point) const;

    static float edge_function(const Vector& point, const Vector& A, const Vector& b);

    bool barycentric_coordinates(const Vector& point, float& u, float& v) const;

    friend std::ostream& operator << (std::ostream& os, const Triangle& triangle);
    friend Triangle operator +(const Triangle& triangle, const Vector& vec);

public:
    Vector _a, _b, _c;
    Vector _normal;

    int _materialIndex;
};

#endif