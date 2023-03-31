#include "sampling.h"
#include "box.h"
#include "naive_nearest.h"
#include "tree.h"
#include "svg.h"

#include "vec.h"

#include <map>
#include <stack>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <cassert>
#include <limits>

using clk = std::chrono::high_resolution_clock ;
using time_point = clk::time_point ;

//===== Running tests =====

//comment to activate / deactivate tests

#define NN_TEST
#define ROOT_NODE_TEST
#define POINT_SPLIT_TEST
#define NODE_SPLIT_TEST
#define TREE_CREATE_TEST
#define TREE_NN_TEST

// tests for the provided box implementation

//#define BOX_BUILD_TEST
//#define BOX_NEAREST_TEST

#if defined TREE_CREATE_TEST || defined TREE_NN_TEST
#define USE_TREE
#endif

#if defined BOX_BUILD_TEST || defined BOX_NEAREST_TEST || defined ROOT_NODE_TEST || defined NODE_SPLIT_TEST || defined TREE_CREATE_TEST || defined TREE_NN_TEST
#define USE_BOX
#endif

//===== Utilities for plotting =====

static constexpr double res = 256 ;
static constexpr double pt_rad = 2 ;
static constexpr const char* stroke_width = "1" ;

std::string color_string(const Color& color) {
  std::stringstream ss ;
  ss << "rgb(" 
    << (int) (color.r * 256) << ","
    << (int) (color.g * 256) << ","
    << (int) (color.b * 256) << ")" ;
  return ss.str() ;
}

SVG::Circle* plot_point(
    SVG::Element* parent, 
    const Point& point, 
    double size = 1
    ) {
  SVG::Circle* c = parent->add_child<SVG::Circle>(
      res * point.x, 
      -res * point.y, 
      size*pt_rad
      ) ;
  c->set_attr("stroke-width", stroke_width) ;
  return c ;
}

SVG::Line* plot_segment(
    SVG::Element* parent, 
    const Point& from, 
    const Point& to
    ) {
  SVG::Line* l = parent->add_child<SVG::Line>(
      res * from.x, 
      res * to.x, 
      -res * from.y, 
      -res * to.y
      ) ;
  l->set_attr("stroke-width", stroke_width) ;
  return l ;
}

#ifdef USE_BOX

SVG::Rect* plot_box(
    SVG::Element* parent, 
    const Box& box 
    ) {
  SVG::Rect* r = parent->add_child<SVG::Rect>(
      res * box.min.x, 
      -res *box.max.y, 
      res * (box.max.x - box.min.x),
      res * (box.max.y - box.min.y)
      ) ;
  r->set_attr("stroke-width", stroke_width) ;
  return r ;
}

#endif

//===== Testing =====

int main() {

  int point_size = 200 ;
  int query_size = 8000 ;

  //2D points for 2D tests
  
  std::vector<Point> points ;

  //use random points in the unit square
  for(int i = 0; i < point_size; ++i) {
    points.push_back(rand_in_square()) ;
  }

#ifdef USE_TREE

  //create the tree before the rest since this reorders the points
  Node tree(points, 0, point_size) ;
  tree.rec_split(points, 10) ;

#endif

  //2D queries for nearest neighbor tests

  std::vector<Point> queries ;

  //use random points in the unit square
  for(int i = 0; i < query_size; ++i) {
    queries.push_back(rand_in_square()) ;
  }

  //random colormap
  
  std::vector<Color> colors ;

  //use random bright colors
  for(int i = 0; i < point_size; ++i) {
    colors.push_back(rand_bright_color()) ;
  }

#ifdef DISTANCE_TEST

  {
    //plot the points colored by distance to the center
    Point center(0.5, 0.5, 0) ;
    
    SVG::SVG svg ;
    SVG::Element* grp = svg.add_child<SVG::Group>() ;

    //points
    for(int i = 0; i < query_size; ++i) {
      double d = 10*distance(queries[i], center) ;
      unsigned int color_index = d ;
      color_index = (color_index * 7) % colors.size() ;

      SVG::Circle* c = plot_point(grp, queries[i]) ;
      c->set_attr("fill", color_string(colors[color_index])) ;
      c->set_attr("stroke", "black") ;
    }

    //export the final svg
    svg.autoscale() ;
    std::ofstream file("distance.svg") ;
    file << std::string(svg) ;

    std::cout << "distance test generated in distance.svg" << std::endl ;
  }

#endif

#ifdef NN_TEST
  
  {

    //perform naive nearest neighbor queries
    std::vector<int> nn(query_size) ;

    time_point naive_start = clk::now() ;
    for(int i = 0; i < query_size; ++i) {
      nn[i] = naive_nearest(points, queries[i]) ;
    }
    time_point naive_end = clk::now() ;
    std::chrono::duration<double> naive_elapsed = naive_end - naive_start ;

    std::cout << "naive nearest neighbor queries performed in " 
      << naive_elapsed.count() << " seconds" << std::endl ;


#ifdef TREE_NN_TEST

    //backup naive nearest_neighbors
    std::vector<int> naive_nn(query_size) ;
    naive_nn.swap(nn) ;

    //perform tree nearest neighbor queries
    time_point tree_start = clk::now() ;
    for(int i = 0; i < query_size; ++i) {
      nn[i] = tree.nearest(points, queries[i]) ;
      //ensure the result is the same as the naive one
      assert(nn[i] == naive_nn[i]) ;
    }
    time_point tree_end = clk::now() ;
    std::chrono::duration<double> tree_elapsed = tree_end - tree_start ;

    std::cout << "tree nearest neighbor queries performed in " 
      << tree_elapsed.count() << " seconds" << std::endl ;

    //ensure the result is the same as the naive one
    for(int i = 0; i < query_size; ++i) {
      assert(nn[i] == naive_nn[i]) ;
    }

#endif

    //plot every point and every query with a color matching the nearest point
    SVG::SVG svg ;
    SVG::Element* grp = svg.add_child<SVG::Group>() ;

    //segments between queries and nearest neighbors
    for(int i = 0; i < query_size; ++i) {
      SVG::Line* segment = plot_segment(grp, queries[i], points[nn[i]]) ;
      segment->set_attr("stroke", color_string(colors[nn[i]])) ;
      segment->set_attr("stroke-width", "0.3") ;
    }

    //queries (smaller than points with no contour)
    for(int i = 0; i < query_size; ++i) {
      SVG::Circle* c = plot_point(grp, queries[i], 0.5) ;
      c->set_attr("fill", color_string(colors[nn[i]])) ;
      c->set_attr("stroke", "none") ;
    }

    //points
    for(int i = 0; i < point_size; ++i) {
      SVG::Circle* c = plot_point(grp, points[i]) ;
      c->set_attr("fill", color_string(colors[i])) ;
      c->set_attr("stroke", "black") ;
    }

    //export the final svg
    svg.autoscale() ;
    std::ofstream file("nn.svg") ;
    file << std::string(svg) ;

    std::cout << "nearest_neighbor test generated in nn.svg" << std::endl ;
  }

#endif

#ifdef BOX_BUILD_TEST

  {
    //create random points in a circle of radius 0.2 centered at (0.3, 0.6)
    double radius = 0.2 ;
    Point center(0.3, 0.6, 0) ;

    std::vector<Point> box_pts ;
    for(int i = 0; i < point_size; ++i) {
      box_pts.push_back(radius * rand_in_disk() + center) ;
    }

    //build a box from the points
    Box box ;
    for(int i = 0; i < point_size; ++i) {
      box.push(box_pts[i]) ;
    }

    //plot the points and their bounding box
    SVG::SVG svg ;
    SVG::Element* grp = svg.add_child<SVG::Group>() ;

    //points
    for(int i = 0; i < point_size; ++i) {
      SVG::Circle* c = plot_point(grp, box_pts[i]) ;
      c->set_attr("fill", color_string(colors[i])) ;
      c->set_attr("stroke", "black") ;
    }

    //box
    SVG::Rect* bb = plot_box(grp, box) ;
    bb->set_attr("fill", "none") ;
    bb->set_attr("stroke", "black") ;

    //bounds
    SVG::Circle* min_pt = plot_point(grp, box.min, 2) ;
    min_pt->set_attr("fill", "none") ;
    min_pt->set_attr("stroke", "black") ;
    SVG::Text* min_lbl = grp->add_child<SVG::Text>(
        res*box.min.x - 3*pt_rad, -res*box.min.y,
        "min"
        ) ;
    min_lbl->set_attr("text-anchor", "end") ;
    SVG::Circle* max_pt = plot_point(grp, box.max, 2) ;
    max_pt->set_attr("fill", "none") ;
    max_pt->set_attr("stroke", "black") ;
    SVG::Text* max_lbl = grp->add_child<SVG::Text>(
        res*box.max.x + 3*pt_rad, -res*box.max.y,
        "max"
        ) ;

    //frame
    Box b ;
    b.push(Point(0,0,0)) ;
    b.push(Point(1,1,0)) ;
    SVG::Rect* r = plot_box(grp, b) ;
    r->set_attr("fill", "none") ;
    r->set_attr("stroke", "none") ;

    //export the final svg
    svg.autoscale() ;
    std::ofstream file("box_build.svg") ;
    file << std::string(svg) ;

    std::cout << "box build test generated in box_build.svg" << std::endl ;
  }

#endif

#ifdef BOX_NEAREST_TEST

  {
    //create a random box
    Box box ;
    box.push(0.3*rand_in_square() + 0.1 * Point(1,1,0)) ;
    box.push(0.3*rand_in_square() + 0.6 * Point(1,1,0)) ;

    //nearest points for each point
    std::vector<Point> box_nn_pts ;
    for(int i = 0; i < point_size; ++i) {
      box_nn_pts.push_back(box.nearest(points[i])) ;
    }

    //plot the points and their nearest neighbors on the box
    SVG::SVG svg ;
    SVG::Element* grp = svg.add_child<SVG::Group>() ;

    //segments between queries and nearest neighbors
    for(int i = 0; i < point_size; ++i) {
      SVG::Line* segment = plot_segment(grp, points[i], box_nn_pts[i]) ;
      segment->set_attr("stroke", "black") ;
    }

    //points
    for(int i = 0; i < point_size; ++i) {
      SVG::Circle* c = plot_point(grp, points[i]) ;
      c->set_attr("fill", color_string(colors[i])) ;
      c->set_attr("stroke", "black") ;
    }

    //box
    SVG::Rect* bb = plot_box(grp, box) ;
    bb->set_attr("fill", "none") ;
    bb->set_attr("stroke", "black") ;

    //nearest points
    for(int i = 0; i < point_size; ++i) {
      SVG::Circle* c = plot_point(grp, box_nn_pts[i], 0.5) ;
      c->set_attr("fill", color_string(colors[i])) ;
      c->set_attr("stroke", "black") ;
      c->set_attr("stroke-width", "0.5") ;
    }

    //frame
    Box b ;
    b.push(Point(0,0,0)) ;
    b.push(Point(1,1,0)) ;
    SVG::Rect* r = plot_box(grp, b) ;
    r->set_attr("fill", "none") ;
    r->set_attr("stroke", "none") ;

    //export the final svg
    svg.autoscale() ;
    std::ofstream file("box_nearest.svg") ;
    file << std::string(svg) ;

    std::cout << "box nearest test generated in box_nearest.svg" << std::endl ;
  }

#endif

#ifdef ROOT_NODE_TEST

  {
    //create random points in two circles of radius 0.2
    double radius = 0.15 ;
    Point center1(0.2, 0.8, 0) ;
    Point center2(0.8, 0.1, 0) ;

    std::vector<Point> box_pts ;

    for(int i = 0; i < point_size / 2; ++i) {
      box_pts.push_back(radius * rand_in_disk() + center1) ;
    }

    for(int i = 0; i < point_size / 2; ++i) {
      box_pts.push_back(radius * rand_in_disk() + center2) ;
    }

    Node tree(box_pts, 0, point_size / 2) ;

    //plot the points and the node bounding box
    SVG::SVG svg ;
    SVG::Element* grp = svg.add_child<SVG::Group>() ;

    //points
    for(std::size_t i = 0; i < box_pts.size(); ++i) {
      SVG::Circle* c = plot_point(grp, box_pts[i]) ;
      c->set_attr("fill", color_string(colors[i])) ;
      c->set_attr("stroke", "black") ;
    }

    //box
    SVG::Rect* bb = plot_box(grp, tree.box()) ;
    bb->set_attr("fill", "none") ;
    bb->set_attr("stroke", "black") ;

    //frame
    Box b ;
    b.push(Point(0,0,0)) ;
    b.push(Point(1,1,0)) ;
    SVG::Rect* r = plot_box(grp, b) ;
    r->set_attr("fill", "none") ;
    r->set_attr("stroke", "none") ;

    //export the final svg
    svg.autoscale() ;
    std::ofstream file("root_node.svg") ;
    file << std::string(svg) ;

    std::cout << "root node test generated in root_node.svg" << std::endl ;
  }

#endif

#ifdef POINT_SPLIT_TEST

  {
    std::vector<Point> split_points ;

    //use random points in the unit square
    for(int i = 0; i < point_size; ++i) {
      split_points.push_back(rand_in_square()) ;
    }

    //split the points in the middle of the x axis
    int sup = partition(0, 0.5, split_points, point_size / 10 , point_size - point_size / 10) ;

    //plot the points with a color depending on their side
    SVG::SVG svg ;
    SVG::Element* grp = svg.add_child<SVG::Group>() ;

    //points
    for(int i = 0; i < point_size; ++i) {
      SVG::Circle* c = plot_point(grp, split_points[i]) ;
      c->set_attr("stroke", "black") ;
      if(i > point_size / 10 && i < sup) {
        c->set_attr("fill", "green") ;
      } else if (i >= sup && i < point_size - point_size / 10){
        c->set_attr("fill", "red") ;
      } else {
        c->set_attr("fill", "yellow") ;
      }
    }

    //split line
    SVG::Line* l = grp->add_child<SVG::Line>(0.5*res, 0.5*res, 0, -res) ;
    l->set_attr("stroke", "black") ;

    //frame
    Box b ;
    b.push(Point(0,0,0)) ;
    b.push(Point(1,1,0)) ;
    SVG::Rect* r = plot_box(grp, b) ;
    r->set_attr("fill", "none") ;
    r->set_attr("stroke", "none") ;

    //export the final svg
    svg.autoscale() ;
    std::ofstream file("point_split.svg") ;
    file << std::string(svg) ;

    std::cout << "point_split test generated in point_split.svg" << std::endl ;
  }

#endif

#ifdef NODE_SPLIT_TEST

  {
    std::vector<Point> split_points ;

    //create points on a first disk
    double radius = 0.1 ;
    Point center1(0.2, 0.8, 0) ;

    for(int i = 0; i < point_size / 4; ++i) {
      split_points.push_back(radius * rand_in_disk() + center1) ;
    }

    //create random points in a rectangle
    for(int i = 0; i < point_size / 2; ++i) {
      split_points.push_back(rand_in_square()) ;
      split_points.back().x *= 0.7 ;
      split_points.back().x += 0.15 ;
      split_points.back().y *= 0.2 ;
      split_points.back().y += 0.4 ;
    }

    //create points on a second disk
    Point center2(0.8, 0.2, 0) ;

    for(int i = 0; i < point_size / 4; ++i) {
      split_points.push_back(radius * rand_in_disk() + center2) ;
    }

    //create the initial node
    Node n(split_points, point_size / 4, point_size / 4 + point_size / 2) ;

    //split the node
    n.split(split_points) ;

    //plot the points and the boxes of the children
    SVG::SVG svg ;
    SVG::Element* grp = svg.add_child<SVG::Group>() ;

    //points:
    for(std::size_t i = 0; i <split_points.size(); ++i) {
      SVG::Circle* pt = plot_point(grp, split_points[i]) ;
      pt->set_attr("stroke", "black") ;
      if(i >= n.child(0)->begin() && i < n.child(0)->end()) {
        //the point is in the first child of the node
        pt->set_attr("fill", color_string(colors[0])) ;
      } else if(i >= n.child(1)->begin() && i < n.child(1)->end()) {
        //the point is in the second child of the node
        pt->set_attr("fill", color_string(colors[10])) ;
      } else {
        //the point is in no child
        pt->set_attr("fill", color_string(colors[i % point_size])) ;
      }
    } 

    for(int i = 0; i < 2; ++i) {
      SVG::Rect* box = plot_box(grp, n.child(i)->box()) ;
      box->set_attr("fill", "none") ;
      box->set_attr("stroke", "black") ;
    }

    //frame
    Box b ;
    b.push(Point(0,0,0)) ;
    b.push(Point(1,1,0)) ;
    SVG::Rect* r = plot_box(grp, b) ;
    r->set_attr("fill", "none") ;
    r->set_attr("stroke", "none") ;

    //export the final svg
    svg.autoscale() ;
    std::ofstream file("node_split.svg") ;
    file << std::string(svg) ;

    std::cout << "node split test generated in node_split.svg" << std::endl ;
  }

#endif

#ifdef TREE_CREATE_TEST

  {
    //display the tree with a depth first traversal
    SVG::SVG svg ;
    SVG::Element* grp = svg.add_child<SVG::Group>() ;

    std::vector<const Node*> stack ;
    stack.push_back(&tree) ;
    while(stack.size() > 0) {
      //pop a node
      const Node* n = stack.back();
      stack.pop_back() ;

      //display the box of the node
      SVG::Rect* box = plot_box(grp, n->box()) ;
      box->set_attr("fill", "none") ;
      box->set_attr("stroke", "black") ;

      if(n->child(0)) {
        //the node has children, push them
        stack.push_back(n->child(0)) ;
        stack.push_back(n->child(1)) ;
      } else {
        //the node is a leaf, display its points with a dedicated color
        std::string color = color_string(rand_bright_color()) ;
        for(int i = n->begin(); i < n->end(); ++i) {
          SVG::Circle* pt = plot_point(grp, points[i]) ;
          pt->set_attr("stroke", "black") ;
          pt->set_attr("fill", color) ;
        }
      }
    }

    //frame
    Box b ;
    b.push(Point(0,0,0)) ;
    b.push(Point(1,1,0)) ;
    SVG::Rect* r = plot_box(grp, b) ;
    r->set_attr("fill", "none") ;
    r->set_attr("stroke", "none") ;

    //export the final svg
    svg.autoscale() ;
    std::ofstream file("tree_create.svg") ;
    file << std::string(svg) ;

    std::cout << "tree create test generated in tree_create.svg" << std::endl ;
  }

#endif

  return 0 ;
}
