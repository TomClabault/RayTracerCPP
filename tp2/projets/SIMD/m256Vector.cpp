#include "m256Vector.h"

__m256Vector::__m256Vector() {};
__m256Vector::__m256Vector(__m256 x, __m256 y, __m256 z) : _x(x), _y(y), _z(z) {};
__m256Vector::__m256Vector(Vector a, Vector b, Vector c, Vector d, Vector e, Vector f, Vector g, Vector h)
{
	float x[8] = { a.x, b.x, c.x, d.x, e.x, f.x, g.x, h.x },
		  y[8] = { a.y, b.y, c.y, d.y, e.y, f.y, g.y, h.y },
		  z[8] = { a.z, b.z, c.z, d.z, e.z, f.z, g.z, h.z };

	_x = _mm256_load_ps(x);
	_y = _mm256_load_ps(y);
	_z = _mm256_load_ps(z);
}
__m256Vector::__m256Vector(Vector vecs[8])
{
	__m256Vector(vecs[0], vecs[1], vecs[2], vecs[3], vecs[4], vecs[5], vecs[6], vecs[7]);
}

Vector __m256Vector::operator[] (int i)
{
#ifdef _MSC_VER //MSVC compiler
    return Vector(_x.m256_f32[i], _y.m256_f32[i], _z.m256_f32[i]);
#elif __GNUC__ //GCC compiler
    return Vector(_x[i], _y[i], _z[i]);
#endif
}

__m256Vector operator / (const __m256Vector& a, float c)
{
	__m256 cVec = _mm256_set1_ps(c);

	return (a / cVec);
}

__m256Vector operator / (const __m256Vector& a, __m256 c)
{
	__m256 ones = _mm256_set1_ps(1);
	__m256 invC = _mm256_div_ps(ones, c);

	__m256Vector divided = __m256Vector(_mm256_mul_ps(a._x, invC), _mm256_mul_ps(a._y, invC), _mm256_mul_ps(a._z, invC));

	return divided;
}

__m256Vector _mm256_cross_product(const __m256Vector& a, const __m256Vector& b)
{
	__m256Vector c;

	__m256 cxLeft = _mm256_mul_ps(a._y, b._z);
	__m256 cxRight = _mm256_mul_ps(a._z, b._y);
	c._x = _mm256_sub_ps(cxLeft, cxRight);

	__m256 cyLeft = _mm256_mul_ps(a._z, b._x);
	__m256 cyRight = _mm256_mul_ps(a._x, b._z);
	c._y = _mm256_sub_ps(cyLeft, cyRight);

	__m256 czLeft = _mm256_mul_ps(a._x, b._y);
	__m256 czRight = _mm256_mul_ps(a._y, b._x);
	c._z = _mm256_sub_ps(czLeft, czRight);

	return c;
}

__m256 _mm256_dot_product(const __m256Vector& a, const __m256Vector& b)
{
	__m256Vector dotRes;

	dotRes._x = _mm256_mul_ps(a._x, b._x);
	dotRes._y = _mm256_mul_ps(a._y, b._y);
	dotRes._z = _mm256_mul_ps(a._z, b._z);

	return _mm256_add_ps(dotRes._x, _mm256_add_ps(dotRes._y, dotRes._z));
}

__m256 _mm256_length(const __m256Vector& a)
{
	__m256 x = _mm256_mul_ps(a._x, a._x);
	__m256 y = _mm256_mul_ps(a._y, a._y);
	__m256 z = _mm256_mul_ps(a._z, a._z);

	return _mm256_sqrt_ps(_mm256_add_ps(x, _mm256_add_ps(y, z)));
}

__m256Vector _mm256_normalize(const __m256Vector& a)
{
	__m256 lengths = _mm256_length(a);

	__m256Vector normalized = a / lengths;

	return normalized;
}
