#include <vector>

#include "bvh.h"

Vector BVH::BoundingVolume::PLANE_NORMALS[7] = {
	Vector(1, 0, 0),
	Vector(0, 1, 0),
	Vector(0, 0, 1),
	Vector(std::sqrt(3) / 3, std::sqrt(3) / 3, std::sqrt(3) / 3),
	Vector(-std::sqrt(3) / 3, std::sqrt(3) / 3, std::sqrt(3) / 3),
	Vector(-std::sqrt(3) / 3, -std::sqrt(3) / 3, std::sqrt(3) / 3),
	Vector(std::sqrt(3) / 3, -std::sqrt(3) / 3, std::sqrt(3) / 3),
};

BVH::BVH(const std::vector<Triangle> triangles, int max_depth)  : _triangles(triangles), _max_depth(max_depth)
{
	BoundingVolume volume;
	Point minimum(INFINITY, INFINITY, INFINITY), maximum(-INFINITY, -INFINITY, -INFINITY);

	for (const Triangle& triangle : triangles)
	{
		volume.extend_volume(triangle);

		for (int i = 0; i < 3; i++)
		{
			minimum = min(minimum, triangle[i]);
			maximum = max(maximum, triangle[i]);
		}
	}

	//TODO replace les std::array du code par des array C style simples
	//We now have a bounding volume to work with
	build_bvh(minimum, maximum, volume);
}

void BVH::build_bvh(Point mini, Point maxi, const BoundingVolume& volume)
{
	_root = new OctreeNode(mini, maxi);

	for (Triangle& triangle : _triangles)
		_root->insert(&triangle, 0, _max_depth);

	_root->compute_volume();
}

bool BVH::intersect(const Ray& ray, HitInfo& hit_info)
{
	float trash;
	//TODO faire une fonction auxiliaire pour cache l'API avec t_near
	return _root->intersect(ray, hit_info, trash);
}

