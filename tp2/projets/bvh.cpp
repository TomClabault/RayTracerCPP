#include <vector>

#include <cmath>

#include "bvh.h"
#include "timer.h"

Vector BVH::BoundingVolume::PLANE_NORMALS[7] = {
	Vector(1, 0, 0),
	Vector(0, 1, 0),
    Vector(0, 0, 1),
    Vector(std::sqrt(3.0f) / 3, std::sqrt(3.0f) / 3, std::sqrt(3.0f) / 3),
    Vector(-std::sqrt(3.0f) / 3, std::sqrt(3.0f) / 3, std::sqrt(3.0f) / 3),
    Vector(-std::sqrt(3.0f) / 3, -std::sqrt(3.0f) / 3, std::sqrt(3.0f) / 3),
    Vector(std::sqrt(3.0f) / 3, -std::sqrt(3.0f) / 3, std::sqrt(3.0f) / 3),
};

BVH::BVH() : _root(nullptr), _triangles(nullptr) {}
BVH::BVH(std::vector<Triangle>* triangles, int max_depth, int leaf_max_obj_count) : _triangles(triangles)
{
	Timer timer;
	timer.start();

	BoundingVolume volume;
	Point minimum(INFINITY, INFINITY, INFINITY), maximum(-INFINITY, -INFINITY, -INFINITY);

	for (const Triangle& triangle : *triangles)
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

	timer.stop();
    //std::cout << "BVH Construction Time: " << timer.elapsed() << "ms\n";
}

BVH::~BVH()
{
	delete _root;
}

void BVH::operator=(BVH&& bvh)
{
	_triangles = bvh._triangles;
	_root = bvh._root;

	bvh._root = nullptr;
}

void BVH::build_bvh(int max_depth, int leaf_max_obj_count, Point min, Point max, const BoundingVolume& volume)
{
	_root = new OctreeNode(min, max);

	for (Triangle& triangle : *_triangles)
		_root->insert(&triangle, 0, max_depth, leaf_max_obj_count);

	_root->compute_volume();
}

bool BVH::intersect(const Ray& ray, HitInfo& hit_info) const
{
	return _root->intersect(ray, hit_info);
}

