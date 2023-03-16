#ifndef MESH_IO_UTILS_H
#define MESH_IO_UTILS_H

#include "mat.h"
#include "mesh_io.h"
#include "triangle.h"

class MeshIOUtils
{
public:
	static std::vector<Triangle> create_triangles(MeshIOData meshData, Transform meshTransform);
};

#endif