#ifndef HIT_INFO_H
#define HIT_INFO_H

#include "vec.h"

class Triangle;

struct HitInfo
{
    const Triangle* triangle = nullptr;

    //Intersection distance to the triangle
    float t = -1;

    //Barycentric coordinates of the intersection point in the triangle
    //P = wA + uB + vC
    float u = 1.0, v = 0.0;

    int mat_index = -1;//Index of the material of the intersected triangle. 
    //This index can be used to retrieve the material in the Materials array in the Scene

    //Normal at the intersection point. Normalized
    Vector normal_at_intersection;
    //Tangent to the normal at the intersection point. This goes along
    //the u texture coordinate and is orthogonal to the geometry normal
    Vector tangent = Vector(0, 0, 0);

    friend std::ostream& operator <<(std::ostream& os, const HitInfo& infos);
};

#endif
