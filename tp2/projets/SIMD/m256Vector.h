#ifndef __M256_VECTOR_H
#define __M256_VECTOR_H

#include <immintrin.h>

#include "vec.h"

class __m256Vector
{
public:
	__m256Vector();
	__m256Vector(__m256 x, __m256 y, __m256 z);
	__m256Vector(Vector a, Vector b, Vector c, Vector d, Vector e, Vector f, Vector g, Vector h);
	__m256Vector(Vector vecs[8]);

	Vector operator[] (int i);
	friend __m256Vector operator / (const __m256Vector& a, float c);
	friend __m256Vector operator / (const __m256Vector& a, __m256 c);

public:
	__m256 _x, _y, _z;
};

__m256 _mm256_length(const __m256Vector& a);
__m256Vector _mm256_normalize(const __m256Vector& a);
__m256Vector _mm256_cross_product(const __m256Vector& a, const __m256Vector& b);
__m256 _mm256_dot_product(const __m256Vector& a, const __m256Vector& b);

#endif
