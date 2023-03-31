#ifndef MIF27_SAMPLING_H
#define MIF27_SAMPLING_H

#include "vec.h"
#include "color.h"

#include <random>

//-- seed random numbers --

//Note that these functions use a common internal random engine and are
//therefore not thread safe.

static std::default_random_engine alea ;

namespace SamplingOptions {
  void use_random_device() ;
}

//-- 1D sampling --

int rand_index(int begin, int end) ;
double rand_double() ;

//-- 2D sampling --

//random point in the unit square
Point rand_in_square() ;

//random point on the unit circle
Point rand_in_circle() ;

//random point in the unit disk
Point rand_in_disk() ;

//-- 3D sampling --

//random point in the unit cube
Point rand_in_cube() ;

//random point on the unit sphere
Point rand_in_sphere() ;

//random point in the unit ball
Point rand_in_ball() ;

//-- Colors --

Color rand_bright_color() ;

#endif
