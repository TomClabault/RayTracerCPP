#ifndef BVH_H
#define BVH_H

#include <array>

#include "triangle.h"
#include "ray.h"

struct OctreeNode
{
	OctreeNode(Point min, Point max) : _min(min), _max(max) {}

	void create_children()
	{
		float middle_x = (_min.x + _max.x) / 2;
		float middle_y = (_min.y + _max.y) / 2;
		float middle_z = (_min.z + _max.z) / 2;

		_children[0] = OctreeNode(_min, Point(middle_x, middle_y, middle_z));
		_children[1] = OctreeNode(Point(middle_x, _min.y, _min.z), Point(_max.x, middle_y, middle_z));
		_children[2] = OctreeNode(_min + Point(0, middle_y, 0), Point(middle_x, _max.y, middle_z));
		_children[3] = OctreeNode(Point(middle_x, middle_y, _min.z), Point(_max.x, _max.y, middle_z));
		_children[4] = OctreeNode(_min + Point(0, 0, middle_z), Point(middle_x, middle_y, _max.z));
		_children[5] = OctreeNode(Point(middle_x, _min.y, middle_z), Point(_max.x, middle_y, _max.z));
		_children[6] = OctreeNode(_min + Point(0, middle_y, middle_z), Point(middle_x, _max.y, _max.z));
		_children[7] = OctreeNode(Point(middle_x, middle_y, middle_z), Point(_max.x, _max.y, _max.z));
	}

	void insert(Triangle* triangle)
	{
		if (_is_leaf)
		{
			_triangles.push_back(triangle);

			if (_triangles.size() > BVH_LEAF_TRIANGLE_COUNT)
			{
				_is_leaf = false;//This node isn't a leaf anymore

				create_children();

				for (Triangle* triangle : _triangles)
					insert_to_children(triangle);

				_triangles.clear();
				_triangles.shrink_to_fit();
			}
		}
		else
			insert_to_children(triangle);

	}

	void insert_to_children(Triangle* triangle)
	{
		Point centroid = triangle->centroid();

		float middle_x = (_min.x + _max.x) / 2;
		float middle_y = (_min.y + _max.y) / 2;
		float middle_z = (_min.z + _max.z) / 2;

		int octant_index = 0;

		if (centroid.x > middle_x) octant_index += 1;
		if (centroid.y > middle_y) octant_index += 2;
		if (centroid.z > middle_z) octant_index += 4;

		_children[octant_index].insert(triangle);
	}

	bool intersect(const Ray& ray, HitInfo& hit_info) const
	{

	}

	//If this node has been subdivided (and thus cannot accept any triangles), 
	//this boolean will be set to false
	bool _is_leaf = 1;

	std::vector<Triangle*> _triangles;
	std::array<OctreeNode, 8> _children;

	Point _min, _max;
};

class BVH
{
public:
	BVH(const std::vector<Triangle> triangles, Point& bounding_volume_min, Point& bounding_volume_max);
	BVH(const std::vector<Triangle> triangles);//TODO changer le constructeur pour ne pas se manger la copie du std::vector en entier à chaque fois et c'est partout dans le code comme ça apparemment (constructeur de Scene aussi par exemple)

	bool intersect(const Ray& ray, HitInfo& hit_info);

private:
	void build_bvh(const Point& min, const Point& max);

public:
	OctreeNode* _root;

	std::vector<Triangle> _triangles;
};

#endif
