#include "mesh_io.h"
#include "objUtils.h"
#include "triangle.h"

std::vector<Triangle> ObjUtils::readObj(const char* filePath, const Transform transform, Vector& pMin, Vector& pMax)
{
    std::vector<Triangle> _triangles;

    std::vector<Point> positions;

    if(!read_positions(filePath, positions))
        return std::vector<Triangle>();       

    printf("%d triangles\n", int(positions.size() / 3));
    //triangles.reserve(int(positions.size() / 3));
   
    // deplace tous les sommets devant la camera
    for (unsigned i = 0; i < positions.size(); i++)
        positions[i] = transform(positions[i]);
   
    // englobant des points, verifier qu'ils sont bien devant la camera...
    Point pmin = positions[0];
    Point pmax = positions[0];
    for(unsigned i= 1; i < positions.size(); i++)
    {
        pmin= min(pmin, positions[i]);
        pmax= max(pmax, positions[i]);
    }

    printf("bounds [%f %f %f]x[%f %f %f]\n", pmin.x, pmin.y, pmin.z, pmax.x, pmax.y, pmax.z);
    pMin = Vector(pmin);
    pMax = Vector(pmax);

    // parcours tous les triangles
    for(unsigned i= 0; i +2 < positions.size(); i+= 3)
    {
        Point p[3]= {
            positions[ i    ],
            positions[ i +1 ],
            positions[ i +2 ]
        };

        _triangles.push_back(Triangle(p[0], p[1], p[2]));
    }

    return _triangles;
}

std::vector<Triangle> ObjUtils::readObj(const char* filePath, const Transform transform)
{
    Vector trash;

    return ObjUtils::readObj(filePath, transform, trash, trash);
}

std::vector<Triangle> ObjUtils::readObj(const char* filePath)
{
    Vector trash;

    return ObjUtils::readObj(filePath, Translation(Vector(0, 0, 0)), trash, trash);
}