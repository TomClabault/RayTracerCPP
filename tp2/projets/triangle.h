#ifndef TRIANGLE_H
#define TRIANGLE_H

#include "ray.h"
#include "vec.h"

//1 to use the Moller Trumbore ray-triangle intersection algorithm. 
//0 to use the naive (barycentric coordinates) ray-triangle intersection algorithm
#define MOLLER_TRUMBORE 1

//Whether or not to render triangles that are facing away from the camera
#define BACKFACE_CULLING 1

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

    friend std::ostream& operator <<(std::ostream& os, const Triangle4& infos);
};

class Triangle
{
public:
    constexpr static double EPSILON = 1.0e-4;

    Triangle();
    Triangle(Point a, Point b, Point c, int material_index = -1);

    /*Triangle(const Triangle& triangle)
    {
        _a = triangle._a;
        _b = triangle._b;
        _c = triangle._c;
        _normal = triangle._normal;
        _materialIndex = triangle._materialIndex;

        std::cout << "copy";
    }*/

    /*
     * Converts a triangle 4 in homogeneous coordinates to a cartesian triangle by dividing each verte by their homogeneous w component
     */
    Triangle(Triangle4 triangle, int material_index = -1);

    bool intersect(const Ray& ray, HitInfo& hitInfo) const;
    bool intersect(const Ray& ray, float& t, float& u, float& v) const;

    /*
     * Inside/outside test considering the triangle's vertices to all have equal z coordinates.
     * This test essentially ignores the z coordinates of the triangle's vertices
     */
    bool inside_outside_2D(const Point& point) const;

    static inline float edge_function(const Point& point, const Point& a, const Point& b)
    {
        return (b.x - a.x) * (point.y - a.y) - (b.y - a.y) * (point.x - a.x);
    }

    bool barycentric_coordinates(const Point& point, float& u, float& v) const;

    Point bbox_centroid() const;

    friend std::ostream& operator << (std::ostream& os, const Triangle& triangle);

    friend Triangle operator +(const Triangle& triangle, const Vector& vec);

    Point& operator[] (int i);
    const Point& operator[] (int i) const;

public:
    Point _a, _b, _c;
    Vector _normal;

    int _materialIndex;
};

#endif
