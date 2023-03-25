#include "meshIOUtils.h"
#include "mesh_io.h"

std::vector<Triangle> MeshIOUtils::create_triangles(MeshIOData meshData, Transform meshTransform)
{
	std::vector<Triangle> _triangles;
	_triangles.reserve(meshData.indices.size() / 3);

	for (size_t i = 0; i < meshData.indices.size(); i += 3)
	{
		int index0 = meshData.indices[i + 0];
		int index1 = meshData.indices[i + 1];
		int index2 = meshData.indices[i + 2];

		Triangle triangle(Point(meshTransform(meshData.positions[index0])),
						  Point(meshTransform(meshData.positions[index1])),
						  Point(meshTransform(meshData.positions[index2])));
		triangle._materialIndex = meshData.material_indices[i / 3];
		_triangles.push_back(triangle);
	}

	return _triangles;
}

std::vector<Triangle> MeshIOUtils::create_triangles(MeshIOData meshData)
{
	return MeshIOUtils::create_triangles(meshData, Identity());
}
