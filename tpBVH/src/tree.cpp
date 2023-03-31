#include "tree.h"
#include <algorithm>

//implémentez vos méthodes ici (voir tree.h)
int partition(int axis, double offset, std::vector<Point>& points, int begin, int end)
{
    std::vector<Point> less_than;
    std::vector<Point> greater_than;

    for (int index = begin; index < end; index++)
    {
        Point point = points[index];

        switch (axis)
        {
            case 0://X
                if (point.x <= offset)
                    less_than.push_back(point);
                else
                    greater_than.push_back(point);

                break;
            case 1://Y
                if (point.y <= offset)
                    less_than.push_back(point);
                else
                    greater_than.push_back(point);

                break;
        }
    }

    int return_position = -1;
    //On parcourt les points qu'on a trie
    for (int index = begin; index < end; index++)
    {
        if (index - begin < less_than.size())
            points[index] = less_than[index - begin];
        else
        {
            if (return_position == -1)
                return_position = index;

            points[index] = greater_than[index - return_position];
        }
    }

    return return_position;
}