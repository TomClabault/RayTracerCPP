#ifndef MIF27_TREE_H
#define MIF27_TREE_H

#include "vec.h"
#include "box.h"

#include <vector>

int partition(int axis, double offset, std::vector<Point>& points, int begin, int end);

struct Node
{
    Node(const std::vector<Point>& points, int start, int end) : 
        _start(start), _end(end), _children({nullptr, nullptr})
    {
        for (int index = start; index < end; index++)
            _bounding_box.push(points[index]);
    }

    int nearest_aux(std::vector<Point>& points, const Point& query, int& candidate_index, float& distance_to_candidate)
    {
        if (!(_children[0] && _children[1]))//C'est une feuille
        {
            int min_index = candidate_index;//Index du candidat, convention que ce soit 0
            float min_distance = distance_to_candidate;

            for (int index = _start; index < _end; index++)
            {
                float dist = distance(points[index], query);

                if (dist < min_distance)
                {
                    min_distance = dist;
                    min_index = index;
                }
            }

            distance_to_candidate = min_distance;
            return min_index;
        }
        else
        {
            float dist_to_first_child = distance(_children[0]->box().nearest(query), query);
            float dist_to_second_child = distance(_children[1]->box().nearest(query), query);

            //Le premier fils est plus proche que le second
            if (dist_to_first_child < dist_to_second_child)
            {
                //Le premier enfant contient potentiellement un point plus proche
                //que notre solution actuelle
                if (dist_to_first_child < distance_to_candidate)
                    candidate_index = _children[0]->nearest_aux(points, query, candidate_index, distance_to_candidate);

                //On va ensuite explorer le deuxième enfant si c'est nécessaire
                if (dist_to_second_child < distance_to_candidate)
                    candidate_index = _children[1]->nearest_aux(points, query, candidate_index, distance_to_candidate);

                return candidate_index;
            }
            else//Le second fils est plus proche que le premier, on va l'explorer d'abord
            {
                //On va explorer le deuxième enfant si c'est nécessaire
                if (dist_to_second_child < distance_to_candidate)
                    candidate_index = _children[1]->nearest_aux(points, query, candidate_index, distance_to_candidate);

                //Si le premier enfant est plus près que notre solution actuelle,
                //même après l'exploration du second fils                    
                if (dist_to_first_child < distance_to_candidate)
                    candidate_index = _children[0]->nearest_aux(points, query, candidate_index, distance_to_candidate);

                return candidate_index;
            }
        }
    }

    int nearest(std::vector<Point>& points, const Point& query)
    {
        int candidate_index = 0;
        float distance_to_candidate = distance(points[candidate_index], query);

        return nearest_aux(points, query, candidate_index, distance_to_candidate);
    }

    void split(std::vector<Point>& points)
    {
        float x_length = _bounding_box.max.x - _bounding_box.min.x;
        float y_length = _bounding_box.max.y - _bounding_box.min.y;

        int middle_pos;
        if (x_length > y_length)//On va découper en 2 le long de l'axe X
        {
            float middle = (_bounding_box.max.x + _bounding_box.min.x) / 2;

            middle_pos = partition(0, middle, points, _start, _end);
        }
        else//On va découper le long de l'axe y
        {
            float middle = (_bounding_box.max.y + _bounding_box.min.y) / 2;

            middle_pos = partition(1, middle, points, _start, _end);
        }

        _children[0] = new Node(points, _start, middle_pos);
        _children[1] = new Node(points, middle_pos, _end);
    }

    void rec_split(std::vector<Point>& points, int max_object_count)
    {
        if (_end - _start > max_object_count)
        {
            split(points);

            _children[0]->rec_split(points, max_object_count);
            _children[1]->rec_split(points, max_object_count);
        }
    }


    int begin() const
    {
        return _start;
    }

    int end() const
    {
        return _end;
    }

    const Box& box() const
    {
        return _bounding_box;
    }

    const Node* child(int i) const
    {
        return _children[i];
    }

    int _start, _end;
    
    Box _bounding_box;

    Node* _children[2];
};

//Q4 : ajoutez :
// - des noeuds enfants comme membres
// - une méthode child(i)
//   . child(0) -> adresse du premier enfant
//   . child(1) -> adresse du second enfant
// - une méthode split qui crée les enfants et répartit les points dedans

//Q5 : ajoutez la méthode rec_split(points, max_points) pour créer l'arbre

//Q6 : ajoutez la méthode nearest(points, query)


//Q3 : implémentez la fonction suivante pour réordonner un tableau
//  
// - axis est l'axe selon lequel les points sont répartis
// - offset est la valeur sur l'axe pour séparer les points
// - points est le tableau à réordonner
// - begin (inclus) et end (exclus) indique la plage du tableau

#endif
