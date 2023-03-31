#include "sampling.h"

#include<cmath>

//===== sampling =====

namespace SamplingOptions {

  //seed the default engine with a random device seed
  void use_random_device() {
    std::random_device dev ;
    alea.seed(dev()) ;
  }
}

//-- 1D sampling --

int rand_index(int begin, int end) {
  std::uniform_int_distribution<int> rand_int(begin, end - 1) ;
  return rand_int(alea) ;
}

double rand_double() {
  std::uniform_real_distribution<double> rand_coord(0,1) ;
  return rand_coord(alea) ;
}

//-- 2D sampling --

//random point in the unit square
Point rand_in_square() 
{
  std::uniform_real_distribution<double> rand_coord(0,1) ;
  return Point(rand_coord(alea), rand_coord(alea), 0) ;
}

//random point on the unit circle
Point rand_in_circle() 
{
  std::normal_distribution<> rand_coord ;
  Point result(rand_coord(alea), rand_coord(alea), 0) ;
  double inv_norm = 1. / distance(result, Origin()) ;
  return inv_norm * result ;
}

//random point in the unit disk
Point rand_in_disk() 
{
  std::uniform_real_distribution<double> rand_radius(0,1) ;
  return sqrt(rand_radius(alea)) * rand_in_circle() ;
}

//-- 3D sampling --

//random point in the unit cube
Point rand_in_cube() 
{
  std::uniform_real_distribution<double> rand_coord(0,1) ;
  return Point(rand_coord(alea), rand_coord(alea), rand_coord(alea)) ;
}

//random point on the unit sphere
Point rand_in_sphere() 
{
  std::normal_distribution<> rand_coord ;
  Point result(rand_coord(alea), rand_coord(alea), rand_coord(alea)) ;
  double inv_norm = 1. / distance(result, Origin()) ;
  return inv_norm * result ;
}

//random point in the unit ball
Point rand_in_ball() 
{
  std::uniform_real_distribution<double> rand_radius(0,1) ;
  return std::pow(rand_radius(alea), 1./3.) * rand_in_sphere() ;
}

//-- Colors --

static Color hsv_to_rgb(const Color& hsv)
{
  Color rgb ;

  double h = hsv.r ;
  double s = hsv.g ;
  double v = hsv.b ;

  int hi = (int) h ;
  double hr = h - hi ;

  double l = v*(1-s) ;
  double m = v*(1-hr*s);
  double n = v*(1-(1-hr)*s);

  if(h<1) {
    rgb.r = v ;
    rgb.g = n ;
    rgb.b = l ;
  } else if(h<2) {
    rgb.r = m ;
    rgb.g = v ;
    rgb.b = l ;
  } else if(h<3) {
    rgb.r = l ;
    rgb.g = v ;
    rgb.b = n ;
  } else if(h<4) {
    rgb.r = l ;
    rgb.g = m ;
    rgb.b = v ;
  } else if(h<5) {
    rgb.r = n ;
    rgb.g = l ;
    rgb.b = v ;
  } else {
    rgb.r = v ;
    rgb.g = l ;
    rgb.b = m ;
  }

  return rgb ;
}

Color rand_bright_color() 
{
  std::uniform_real_distribution<double> rand_double(0,1) ;

  Color hsv ;
  //random hue
  hsv.r = rand_double(alea) ;
  hsv.r *= 6 ;
  //random saturation, with minimum at 0.5
  hsv.g = rand_double(alea) ;
  hsv.g = 0.7*hsv.g + 0.3 ;
  //random value with minimum at 0.5
  hsv.b = rand_double(alea) ;
  hsv.b = 0.2*hsv.b+0.8 ;

  //RBG conversion
  return hsv_to_rgb(hsv) ;
}
