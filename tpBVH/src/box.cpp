#include "box.h"

#include <limits>
#include <algorithm>

//an empty box can be detected by the fact that max < min
Box::Box() : 
  min(std::numeric_limits<float>::infinity(),
      std::numeric_limits<float>::infinity(),
      std::numeric_limits<float>::infinity()),
  max(-std::numeric_limits<float>::infinity(),
      -std::numeric_limits<float>::infinity(),
      -std::numeric_limits<float>::infinity())
{}

void Box::push(const Point& point) {
  min.x = min.x < point.x ? min.x : point.x ;
  min.y = min.y < point.y ? min.y : point.y ;
  min.z = min.z < point.z ? min.z : point.z ;
  max.x = max.x > point.x ? max.x : point.x ;
  max.y = max.y > point.y ? max.y : point.y ;
  max.z = max.z > point.z ? max.z : point.z ;
}

Point Box::nearest(const Point& point) const {
  Point result ;
  for(int c = 0; c < 3; ++c) {
    result(c) = std::min(std::max(point(c), min(c)), max(c)) ;
  }
  return result ;
}
