#include <vector>

#include <cmath>

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

BVH::BVH(const std::vector<Triangle> triangles, int max_depth, int leaf_max_obj_count) : _triangles(triangles)
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

	//We now have a bounding volume to work with
	build_bvh(max_depth, leaf_max_obj_count, minimum, maximum, volume);
}

void BVH::build_bvh(int max_depth, int leaf_max_obj_count, Point min, Point max, const BoundingVolume& volume)
{
	_root = new OctreeNode(min, max, max_depth, leaf_max_obj_count);

	for (Triangle& triangle : _triangles)
		_root->insert(&triangle, 0);

	_root->compute_volume();
}

bool BVH::intersect(const Ray& ray, HitInfo& hit_info) const
{
	return _root->intersect(ray, hit_info);
}

