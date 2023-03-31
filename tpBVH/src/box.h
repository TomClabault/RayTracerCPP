#ifndef MIF27_BOX_H
#define MIF27_BOX_H

#include "vec.h"

struct Box {
  Box() ;

  //Adding points to the box
  void push(const Point& p) ;

  //Distance from a point to the box
  Point nearest(const Point& p) const ;

  //Bounds of the box
  Point min ;
  Point max ;
} ;

#endif
