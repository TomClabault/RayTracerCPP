#ifndef M256POINT_H
#define M256POINT_H

#include "mat.h"
#include "m256Vector.h"

#include <immintrin.h>

struct __m256Point
{
	__m256Point(__m256 a, __m256 b, __m256 c) : _x(a), _y(b), _z(c) {}
	__m256Point(const __m256Vector& vec) : _x(vec._x), _y(vec._y), _z(vec._z) {}

	friend __m256Point operator + (const __m256Point& a, const __m256Point& b);
	friend __m256Vector operator - (const __m256Point& a, const __m256Point& b);
	friend __m256Point operator * (const __m256Point& a, __m256 k);
	friend __m256Point operator * (__m256 k, const __m256Point& a);

    __m256Point transform(const Transform& transfo) const;

	__m256 _x, _y, _z;
};

#endif // !M256POINT_H

