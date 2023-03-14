#ifndef __M256_TRIANGLES_H
#define __M256_TRIANGLES_H

#include <immintrin.h>

#include "m256Vector.h"

class __m256Triangles
{
public:
    constexpr static double EPSILON = 1.0e-4;

private:
    __m256Vector _a, _b, _c;
};

#endif
