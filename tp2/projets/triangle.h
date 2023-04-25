#ifndef TRIANGLE_H
#define TRIANGLE_H

#include "hitInfo.h"
#include "ray.h"
#include "vec.h"

//1 to use the Moller Trumbore ray-triangle intersection algorithm. 
//0 to use the naive (barycentric coordinates) ray-triangle intersection algorithm
#define MOLLER_TRUMBORE 1

//Whether or not to render triangles that are facing away from the camera
#define BACKFACE_CULLING 1

/**
 *  -------- UV Coordinates Convention Used --------
 *  A point on a triangle is P = wA + uB + vC
 *  u and v are in [0, 1].
 *  [u, v] = [0, 0] corresponds to the bottom left corner of a texture.
 *  -------- UV Coordinates Convention Used --------
 */

//Triangle with homogeneous coordinates vertices
struct Triangle4
{
    Triangle4() {}
    Triangle4(const vec4&  a, const vec4&  b, const vec4&  c) : _a(a), _b(b), _c(c) {}
    Triangle4(const vec4& a, const vec4&  b, const vec4&  c, const Point& tex_coords_u, const Point& tex_coords_v)
        : _a(a), _b(b), _c(c), _tex_coords_u(tex_coords_u), _tex_coords_v(tex_coords_v) {}

    vec4 _a, _b, _c;

    //Organized as:
    //_tex_coords_u[0], _tex_coords_u[1], _tex_coords_u[2] = A.u, B.u, C.u respectively
    //a,d
    //_tex_coords_v[0], _tex_coords_v[1], _tex_coords_v[2] = A.v, B.v, C.v respectively
    Point _tex_coords_u, _tex_coords_v;

    friend std::ostream& operator <<(std::ostream& os, const Triangle4& infos);
};

class Triangle
{
public:
    constexpr static double EPSILON = 1.0e-4;

    Triangle();
    Triangle(Point a, Point b, Point c, int material_index = -1, const Point& tex_coords_u = Point(-1, -1, -1), const Point& tex_coords_v = Point(-1, -1, -1));

    /*
     * Converts a triangle 4 in homogeneous coordinates to a cartesian
     * triangle by dividing each verte by their homogeneous w component
     */
    Triangle(const Triangle4& triangle, int material_index = -1, const Point& tex_coords_u = Point(-1, -1, -1), const Point& tex_coords_v = Point(-1, -1, -1));

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

    Vector get_tangent(const Vector& ab, const Vector& ac) const;

    /**
     * @brief Given UV barycentric coordinates, computes and stores the UV texture coordinates
     * of the same point in \param out_tex_u and \param out_tex_v
     * @param [in] u Barycentric coordinate u
     * @param [in] v Barycentric coordinate v
     * @param [out] out_tex_u Texture coordinate u
     * @param [out] out_tex_v Texture coordinate v
     */
    void interpolate_texcoords(float u, float v, float& out_tex_u, float& out_tex_v) const;

    Point bbox_centroid() const;

    friend std::ostream& operator << (std::ostream& os, const Triangle& triangle);

    friend Triangle operator +(const Triangle& triangle, const Vector& vec);

    Point& operator[] (int i);
    const Point& operator[] (int i) const;

public:
    Point _a, _b, _c;
    Vector _normal;

    int _materialIndex;

    //Organized as:
    //_tex_coords_u[0], _tex_coords_u[1], _tex_coords_u[2] = A.u, B.u, C.u respectively
    //a,d
    //_tex_coords_v[0], _tex_coords_v[1], _tex_coords_v[2] = A.v, B.v, C.v respectively
    Point _tex_coords_u, _tex_coords_v;
};

#endif
