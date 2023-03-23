#include <vector>

#include "bvh.h"

BVH::BVH(const std::vector<Triangle> triangles, Point& bounding_volume_min, Point& bounding_volume_max)
{
	//This means that the bounding volume is uninitialized
	if (   bounding_volume_min.x == INFINITY && bounding_volume_min.y == INFINITY && bounding_volume_min.z == INFINITY
		&& bounding_volume_max.x == -INFINITY && bounding_volume_max.y == -INFINITY && bounding_volume_max.z == -INFINITY)
	{
		for (const Triangle& triangle : triangles)
		{
			bounding_volume_min = min(bounding_volume_min, triangle._a);
			bounding_volume_min = min(bounding_volume_min, triangle._b);
			bounding_volume_min = min(bounding_volume_min, triangle._c);

			bounding_volume_max = max(bounding_volume_max, triangle._a);
			bounding_volume_max = max(bounding_volume_max, triangle._b);
			bounding_volume_max = max(bounding_volume_max, triangle._c);
		}
	}

	//We now have a bounding volume to work with
	build_bvh();
}

BVH::BVH(const std::vector<Triangle> triangles) : BVH(triangles, Point(INFINITY, INFINITY, INFINITY), Point(-INFINITY, -INFINITY, -INFINITY)) { }

void BVH::build_bvh(const Point& min, const Point& max)
{
	_root = new OctreeNode(min, max);

	for (Triangle& triangle : _triangles)
		_root->insert(&triangle);
}

bool BVH::intersect(const Ray& ray, HitInfo& hit_info)
{
	return _root->intersect(ray, hit_info);
}

