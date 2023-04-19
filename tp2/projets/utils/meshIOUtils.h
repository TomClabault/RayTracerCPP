#ifndef MESH_IO_UTILS_H
#define MESH_IO_UTILS_H

#include "mat.h"
#include "mesh_io.h"
#include "triangle.h"

class MeshIOUtils
{
public:
    static std::vector<Triangle> create_triangles(const MeshIOData& meshData);

    /**
     * @brief Creates the triangles along with their materials, uv coordinates...
     * based on the given mesh data
     * @param meshData The mesh data
     * @param current_material_count This is used as an offset for the material index
     * of each triangle. This argument is useful when you're adding triangles to
     * a renderer that already has materials of previous objects. Leave at 0
     * if no material already exists in the renderer when calling this function
     * @param meshTransform The transform to apply to each triangle
     * @return
     */
    static std::vector<Triangle> create_triangles(const MeshIOData& meshData, int current_material_count, const Transform& meshTransform);
};

#endif
