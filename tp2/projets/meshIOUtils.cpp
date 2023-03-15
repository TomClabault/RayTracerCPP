#include "meshIOUtils.h"
#include "mesh_io.h"

std::vector<Triangle> MeshIOUtils::create_triangles(MeshIOData meshData, Transform meshTransform)
{
	std::vector<Triangle> _triangles;
	_triangles.reserve(meshData.indices.size() / 3);

	for (unsigned int i = 0; i < meshData.indices.size(); i += 3)
	{
		int index0 = meshData.indices[i + 0];
		int index1 = meshData.indices[i + 1];
		int index2 = meshData.indices[i + 2];

		Triangle triangle(Vector(meshTransform(meshData.positions[index0])),
						  Vector(meshTransform(meshData.positions[index1])),
						  Vector(meshTransform(meshData.positions[index2])));
		triangle._materialIndex = meshData.material_indices[i / 3];
		_triangles.push_back(triangle);
	}

	return _triangles;
}
