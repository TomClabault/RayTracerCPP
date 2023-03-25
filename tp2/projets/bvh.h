#ifndef BVH_H
#define BVH_H

#include <array>
#include <cmath>
#include <limits>
#include <queue>

#include "triangle.h"
#include "ray.h"

class BVH
{
public:
	struct BoundingVolume
	{
		static Vector PLANE_NORMALS[7];

        float _d_near[7] = { INFINITY, INFINITY, INFINITY, INFINITY, INFINITY, INFINITY, INFINITY };
        float _d_far[7] = { -INFINITY, -INFINITY, -INFINITY, -INFINITY, -INFINITY, -INFINITY, -INFINITY };

		static void triangle_volume(const Triangle& triangle, float d_near[7], float d_far[7])
		{
			for (int i = 0; i < 7; i++)
			{
				for (int j = 0; j < 3; j++)
				{
					float dist = dot(BoundingVolume::PLANE_NORMALS[i], Vector(triangle[j]));

					d_near[i] = std::min(d_near[i], dist);
					d_far[i] = std::max(d_far[i], dist);
				}
			}
		}

		void extend_volume(const float d_near[7], const float d_far[7])
		{
			for (int i = 0; i < 7; i++)
			{
				_d_near[i] = std::min(_d_near[i], d_near[i]);
				_d_far[i] = std::max(_d_far[i], d_far[i]);
			}
		}

		void extend_volume(const BoundingVolume& volume)
		{
			extend_volume(volume._d_near, volume._d_far);
		}

		void extend_volume(const Triangle& triangle)
		{
            float d_near[7] = { INFINITY, INFINITY, INFINITY, INFINITY, INFINITY, INFINITY, INFINITY };
            float d_far[7] = { -INFINITY, -INFINITY, -INFINITY, -INFINITY, -INFINITY, -INFINITY, -INFINITY };

			triangle_volume(triangle, d_near, d_far);
			extend_volume(d_near, d_far);
		}

		bool intersect(const Ray& ray, float& t_near, float& t_far) const
		{
            t_near = -INFINITY;
            t_far = INFINITY;

			for (int i = 0; i < 7; i++)
			{
				//TODO: precompute denom
				float denom = dot(BoundingVolume::PLANE_NORMALS[i], ray._direction);
				if (denom == 0.0)
					continue;

				//TODO precompute numerateur
				float d_near_i = (_d_near[i] - dot(BoundingVolume::PLANE_NORMALS[i], Vector(ray._origin))) / denom;
				float d_far_i = (_d_far[i] - dot(BoundingVolume::PLANE_NORMALS[i], Vector(ray._origin))) / denom;
				if (denom < 0)
					std::swap(d_near_i, d_far_i);

				t_near = std::max(t_near, d_near_i);
				t_far = std::min(t_far, d_far_i);

				if (t_far < t_near)
					return false;
			}


			return true;
		}
	};

	struct OctreeNode
	{
		struct QueueElement
		{
			QueueElement(const BVH::OctreeNode* node, float t_near) : _node(node), _t_near(t_near) {}

			bool operator > (const QueueElement& a) const
			{
				return _t_near > a._t_near;
			}

			const OctreeNode* _node;//Reference on the node

			float _t_near;//Intersection distance used to order the elements in the priority queue used
			//by the OctreeNode to compute the intersection with a ray
		};

        OctreeNode(Point min, Point max, int max_depth, int leaf_max_obj_count) : _leaf_max_obj_count(leaf_max_obj_count),
            _max_depth(max_depth), _min(min), _max(max) {}

 		 /*
		  * Once the objects have been inserted in the hierarchy, this function computes
		  * the bounding volume of all the node in the hierarchy
		  */
		BoundingVolume compute_volume()
		{
			if (_is_leaf)
				for (const Triangle* triangle : _triangles)
					_bounding_volume.extend_volume(*triangle);
			else
				for (int i = 0; i < 8; i++)
					_bounding_volume.extend_volume(_children[i]->compute_volume());

			return _bounding_volume;
		}

		void create_children()
		{
			float middle_x = (_min.x + _max.x) / 2;
			float middle_y = (_min.y + _max.y) / 2;
			float middle_z = (_min.z + _max.z) / 2;

			_children[0] = new OctreeNode(_min, Point(middle_x, middle_y, middle_z), _max_depth, _leaf_max_obj_count);
			_children[1] = new OctreeNode(Point(middle_x, _min.y, _min.z), Point(_max.x, middle_y, middle_z), _max_depth, _leaf_max_obj_count);
			_children[2] = new OctreeNode(_min + Point(0, middle_y, 0), Point(middle_x, _max.y, middle_z), _max_depth, _leaf_max_obj_count);
			_children[3] = new OctreeNode(Point(middle_x, middle_y, _min.z), Point(_max.x, _max.y, middle_z), _max_depth, _leaf_max_obj_count);
			_children[4] = new OctreeNode(_min + Point(0, 0, middle_z), Point(middle_x, middle_y, _max.z), _max_depth, _leaf_max_obj_count);
			_children[5] = new OctreeNode(Point(middle_x, _min.y, middle_z), Point(_max.x, middle_y, _max.z), _max_depth, _leaf_max_obj_count);
			_children[6] = new OctreeNode(_min + Point(0, middle_y, middle_z), Point(middle_x, _max.y, _max.z), _max_depth, _leaf_max_obj_count);
			_children[7] = new OctreeNode(Point(middle_x, middle_y, middle_z), Point(_max.x, _max.y, _max.z), _max_depth, _leaf_max_obj_count);
		}

		void insert(Triangle* triangle, int current_depth)
		{
			bool depth_exceeded = current_depth == _max_depth;

			if (_is_leaf || depth_exceeded)
			{
				_triangles.push_back(triangle);

				if (_triangles.size() > _leaf_max_obj_count && !depth_exceeded)
				{
					_is_leaf = false;//This node isn't a leaf anymore

					create_children();

					for (Triangle* triangle : _triangles)
						insert_to_children(triangle, current_depth);

					_triangles.clear();
					_triangles.shrink_to_fit();
				}
			}
			else
				insert_to_children(triangle, current_depth);

		}

		void insert_to_children(Triangle* triangle, int current_depth)
		{
			Point bbox_centroid = triangle->bbox_centroid();

			float middle_x = (_min.x + _max.x) / 2;
			float middle_y = (_min.y + _max.y) / 2;
			float middle_z = (_min.z + _max.z) / 2;

			int octant_index = 0;

			if (bbox_centroid.x > middle_x) octant_index += 1;
			if (bbox_centroid.y > middle_y) octant_index += 2;
			if (bbox_centroid.z > middle_z) octant_index += 4;

			_children[octant_index]->insert(triangle, current_depth + 1);
		}

		bool intersect(const Ray& ray, HitInfo& hit_info, float& t_near) const
		{
			float t_far, trash;

			if (!_bounding_volume.intersect(ray, trash, t_far))
				return false;

			if (_is_leaf)
			{
				for (Triangle* triangle : _triangles)
				{
					HitInfo local_hit_info;
					if (triangle->intersect(ray, local_hit_info))
						if (local_hit_info.t < hit_info.t || hit_info.t == -1)
							hit_info = local_hit_info;
				}

				t_near = hit_info.t;

				return t_near > 0;
			}

			std::priority_queue<QueueElement, std::vector<QueueElement>, std::greater<QueueElement>> intersection_queue;
			for (int i = 0; i < 8; i++)
			{
				float inter_distance;
				if (_children[i]->_bounding_volume.intersect(ray, inter_distance, t_far))
					intersection_queue.emplace(QueueElement(_children[i], inter_distance));
			}

            float closest_inter = INFINITY, inter_distance = INFINITY;
			while (!intersection_queue.empty())
			{
				QueueElement top_element = intersection_queue.top();
				intersection_queue.pop();

				if (top_element._node->intersect(ray, hit_info, inter_distance))
				{
					closest_inter = std::min(closest_inter, inter_distance);

					//If we found an intersection that is closer than 
					//the next element in the queue, we can stop intersecting further
					if (intersection_queue.empty() || closest_inter < intersection_queue.top()._t_near)
					{
						t_near = closest_inter;

						return true;
					}
				}
			}

            if (closest_inter == INFINITY)
				return false;
			else
			{
				t_near = closest_inter;

				return true;
			}
		}

		//If this node has been subdivided (and thus cannot accept any triangles), 
		//this boolean will be set to false
		bool _is_leaf = true;

		int _leaf_max_obj_count, _max_depth;

		std::vector<Triangle*> _triangles;
		BVH::OctreeNode* _children[8];

		Point _min, _max;
		BVH::BoundingVolume _bounding_volume;
	};

public:
	//TODO changer le constructeur pour ne pas se manger la copie du std::vector en entier à chaque fois et c'est partout dans le code comme ça apparemment (constructeur de Scene aussi par exemple)
	//TODO tester la best max_depth
	BVH(const std::vector<Triangle> triangles, int max_depth = 10, int leaf_max_obj_count = 8);

	bool intersect(const Ray& ray, HitInfo& hit_info) const;

private:
	void build_bvh(int max_depth, int leaf_max_obj_count, Point min, Point max, const BoundingVolume& volume);

public:
	int _max_depth;

	OctreeNode* _root;

	std::vector<Triangle> _triangles;
};

#endif
