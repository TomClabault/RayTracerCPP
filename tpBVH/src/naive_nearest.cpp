#include "naive_nearest.h"

#include <limits>

int naive_nearest(const std::vector<Point>& points, const Point& query) {
  	int min_point_index = -1;
  	float min_distance = std::numeric_limits<float>::infinity();

  	for (int index = 0; index < points.size(); index++)
  	{
		const Point& point = points[index];

	    float dist = distance(query, point);
	    if (dist < min_distance)
	    {
	      	min_distance = dist;
	  		min_point_index = index;
    	}
  	}

  	return min_point_index;
}
