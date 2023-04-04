#include "m256Point.h"

__m256Point __m256Point::transform(const Transform& transfo) const
{
    __m256 m00 = _mm256_set1_ps(transfo.data()[0]);
    __m256 m01 = _mm256_set1_ps(transfo.data()[1]);
    __m256 m02 = _mm256_set1_ps(transfo.data()[2]);
    __m256 m03 = _mm256_set1_ps(transfo.data()[3]);
    __m256 xt = _mm256_fmadd_ps(m00, _x, _mm256_fmadd_ps(m01, _y, _mm256_fmadd_ps(m02, _z, m03)));        // dot(vec4(m[0]), vec4(p, 1))

    __m256 m10 = _mm256_set1_ps(transfo.data()[4 + 0]);
    __m256 m11 = _mm256_set1_ps(transfo.data()[4 + 1]);
    __m256 m12 = _mm256_set1_ps(transfo.data()[4 + 2]);
    __m256 m13 = _mm256_set1_ps(transfo.data()[4 + 3]);
    __m256 yt = _mm256_fmadd_ps(m10, _x, _mm256_fmadd_ps(m11, _y, _mm256_fmadd_ps(m12, _z, m13)));        // dot(vec4(m[1]), vec4(p, 1))

    __m256 m20 = _mm256_set1_ps(transfo.data()[8 + 0]);
    __m256 m21 = _mm256_set1_ps(transfo.data()[8 + 1]);
    __m256 m22 = _mm256_set1_ps(transfo.data()[8 + 2]);
    __m256 m23 = _mm256_set1_ps(transfo.data()[8 + 3]);
    __m256 zt = _mm256_fmadd_ps(m20, _x, _mm256_fmadd_ps(m21, _y, _mm256_fmadd_ps(m22, _z, m23)));        // dot(vec4(m[2]), vec4(p, 1))

    __m256 m30 = _mm256_set1_ps(transfo.data()[12 + 0]);
    __m256 m31 = _mm256_set1_ps(transfo.data()[12 + 1]);
    __m256 m32 = _mm256_set1_ps(transfo.data()[12 + 2]);
    __m256 m33 = _mm256_set1_ps(transfo.data()[12 + 3]);
    __m256 wt = _mm256_fmadd_ps(m30, _x, _mm256_fmadd_ps(m31, _y, _mm256_fmadd_ps(m32, _z, m33)));        // dot(vec4(m[3]), vec4(p, 1))

    __m256 w = _mm256_div_ps(_mm256_set1_ps(1), wt);

    return __m256Point(_mm256_mul_ps(xt, w), _mm256_mul_ps(yt, w), _mm256_mul_ps(zt, w));
}

__m256Point operator + (const __m256Point& a, const __m256Point& b)
{
	return __m256Point(_mm256_add_ps(a._x, b._x),
					   _mm256_add_ps(a._y, b._y), 
					   _mm256_add_ps(a._z, b._z));
}

__m256Vector operator - (const __m256Point& a, const __m256Point& b)
{
	return __m256Vector(_mm256_sub_ps(a._x, b._x), 
						_mm256_sub_ps(a._y, b._y), 
						_mm256_sub_ps(a._z, b._z));
}

__m256Point operator * (const __m256Point& a, __m256 k)
{
	return __m256Point(_mm256_mul_ps(a._x, k), 
					   _mm256_mul_ps(a._y, k), 
					   _mm256_mul_ps(a._z, k));
}

__m256Point operator * (__m256 k, const __m256Point& a)
{
	return a * k;
}