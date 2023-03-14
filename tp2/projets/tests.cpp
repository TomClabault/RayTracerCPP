#include <iostream>
#include <vector>

#include "mat.h"
#include "mesh_io.h"
#include "meshIOUtils.h"
#include "objUtils.h"
#include "triangle.h"
#include "m256Triangles.h"
#include "m256Vector.h"

#define epsilon 1.0e-5

#define assert_true(predicate, messageOnError) if(!(predicate)) { std::cout << messageOnError; std::exit(0); }

//Macro that tests the equality of 2 floats. If the expected float is 0.0, does a proper checking using epsilon
#define expect_float(u, expected) ((expected > -epsilon && expected < epsilon) ? (u > -epsilon && u < epsilon) : (u == expected))

bool float_equal(float a, float b, float threshold)
{
    float diff = std::abs(a - b);
    
    return (diff > -threshold) && (diff < threshold);
}

bool vector_equal(const Vector & a, const Vector & b, float threshold)
{
    return float_equal(a.x, b.x, threshold) && float_equal(a.y, b.y, threshold) && float_equal(a.z, b.z, threshold);
}

void testBarycentric(Triangle triangle, Vector point, float expected_u, float expected_v, bool expected_in_triangle)
{
    float u, v;

    if (expected_in_triangle) {
        assert_true(triangle.barycentric_coordinates(point, u, v), "Barycentric coordinates detected that the point " << point << " wasn't in the triangle " << triangle << " but it should be" << std::endl);

        assert_true(expect_float(u, expected_u), "Incorrect barycentric coordinate computation. For the triangle " << triangle << " and the point " << point << ", the coefficient u wasn't " << expected_u << " but was " << u << " when calculating the barycentric coordinates for a point in the triangle equal to A." << std::endl);
        assert_true(expect_float(v, expected_v), "Incorrect barycentric coordinate computation. For the triangle " << triangle << " and the point " << point << ", the coefficient v wasn't " << expected_v << " but was " << v << " when calculating the barycentric coordinates for a point in the triangle equal to A." << std::endl);
    }
    else {
        assert_true(!triangle.barycentric_coordinates(point, u, v), "Barycentric coordinates detected that the point " << point << " was in the triangle " << triangle << " but it shouldn't." << std::endl);
    }

}

void barycentric_coordinates_tests()
{
    std::cout << "Testing barycentric coordinates... ";

    Triangle triangleA(Vector(0, 0, 0), Vector(1, 0, 0), Vector(0, 1, 0));
    Triangle triangleE(Vector(-3, 0, -4), Vector(-2, 0, -4), Vector(-2, 1, -4));
    Triangle triangleF(Vector(-3, 0, -4), Vector(-2, 0, -4), Vector(-3, 1, -4));

    float a, b, c;

    testBarycentric(triangleA, Vector(0, 0, 0), 0.0, 0.0, true);
    testBarycentric(triangleE, Vector(-3, 0, -4), 0.0, 0.0, true);
    testBarycentric(triangleF, Vector(-4, 1, -4), 0.0, 0.0, false);

    std::cout << "OK!" << std::endl << std::endl;
}

//void triangle_intersections_tests()
//{
//    std::cout << "Testing triangle intersections... ";
//
//    MeshIOData rubikData 
//    std::vector<Triangle> rubik = ObjUtils::readObj("data/rubik.obj", Vector(-15, -20, -30));
//    std::vector<Triangle> robot = ObjUtils::readObj("data/robot.obj", Vector(0, -2, -4));
//
//    Triangle triangleA(Vector(0, 0, 0), Vector(1, 0, 0), Vector(0, 1, 0));
//    Triangle triangleB(Vector(1, -1, -9), Vector(-1, -1, -9), Vector(-1, -1, -11));
//    Triangle triangleC(Vector(-1, 1, -11), Vector(-1, 1, -9), Vector(1, 1, -9));
//
//    Ray ray00(Vector(0, 0, 0), Vector(-0.577350259, 0.577350259, -0.577350259));
//    Ray ray(Vector(0, 0, -1), Vector(0, 0, 1));
//    Ray ray2(Vector(0, 0, 0), Vector(0.103264742, 0.312963158, -0.944134772));
//    Ray rayOut(Vector(-2, 0, -1), Vector(0, 0, 1));
//    Ray ray200x200(Vector(0, 0, 0), normalize(Vector(200.0 / 1919.0 * 2 - 1, (1079 - 200.0) / 1079.0 * 2 - 1, -1)));
//
//    float t, trash;
//
//    assert_true(triangleA.intersect(ray, t, trash, trash), ray << " intersecting " << triangleA << " but shouldn't because of back face culling\n");
//    assert_true(t == 1.0, "Intersection distance of " << ray << " with " << triangleA << " should be -1 but is " << t);
//
//    assert_true(!triangleA.intersect(rayOut, t, trash, trash), rayOut << " intersecting " << triangleA << " but shouldn't\n");
//    assert_true(!triangleB.intersect(ray00, t, trash, trash), ray00 << " intersecting " << triangleB << " but shouldn't\n");
//    assert_true(!triangleC.intersect(ray00, t, trash, trash), ray00 << " intersecting " << triangleC << " but shouldn't\n");
//
//    float u, v, c;
//    for (Triangle& triangle : rubik) {
//        assert_true(!triangle.intersect(ray200x200, t, u, v), "Ray " << ray200x200 << " intersected with the triangle " << triangle << " of the rubik but shouldn't have." << std::endl);
//    }
//
//    for (Triangle& triangle : robot) {
//        assert_true(!triangle.intersect(ray200x200, t, u, v), "Ray " << ray200x200 << " intersected with the triangle " << triangle << " of the robot but shouldn't have." << std::endl);
//    }
//
//    std::cout << "OK!" << std::endl << std::endl;
//}

void SIMD_implementations_tests()
{
    Vector a = Vector(1, 0, 0);
    Vector b = Vector(0, 1, 0);
    Vector c = Vector(0, 0, 1);
    Vector d = Vector(-0.577350259, 0.577350259, -0.577350259);
    Vector e = Vector(0.103264742, 0.312963158, -0.944134772);
    Vector f = Vector(-1, 1, -11);
    Vector g = Vector(-1, 1, -9);
    Vector h = Vector(1, 1, -9);

    Vector i = -a + c;
    Vector j = -b + d;
    Vector k = -c + e;
    Vector l = -d + f;
    Vector m = -e + g;
    Vector n = -f + h;
    Vector o = -g + i;
    Vector p = -h + j;

    __m256Vector __a(a, b, c, d, e, f, g, h);
    __m256Vector __b(i, j, k, l, m, n, o, p);

    srand(time(NULL));
    float random_float = (rand() / (float)RAND_MAX) * (RAND_MAX / 4.0);

    std::cout << "Testing SIMD / operator(float)... ";
    __m256Vector __aDiv = __a / random_float;
    for (int i = 0; i < 8; i++) {
        assert_true(vector_equal(__aDiv[i], __a[i] / random_float, epsilon), "SIMD / operator(float) wasn't equal to reference / operator at index " << i << " with float " << random_float << ". Was " << __aDiv[i] << " but expected " << __a[i] / random_float << std::endl);
    }
    std::cout << "OK!" << std::endl;
    // -------------------------------------------------------------------- //
    // -------------------------------------------------------------------- //
    // --------------- //
    std::cout << "Testing SIMD / operator(__m256)... ";
    __m256Vector __bDiv = __b / _mm256_set1_ps(random_float);
    for (int i = 0; i < 8; i++) {
        assert_true(vector_equal(__bDiv[i], __b[i] / random_float, epsilon), "SIMD / operator(float) wasn't equal to reference / operator at index " << i << " with float " << random_float << ". Was " << __bDiv[i] << " but expected " << __b[i] / random_float << std::endl);
    }
    std::cout << "OK!" << std::endl;
    // --------------- //
    // -------------------------------------------------------------------- //
    // -------------------------------------------------------------------- //
    // --------------- //
    std::cout << "Testing SIMD lengths... ";
    __m256 lengthsA = _mm256_length(__a);
    __m256 lengthsB = _mm256_length(__b);

    for (int i = 0; i < 8; i++) {
        assert_true(float_equal(lengthsA.m256_f32[i], length(__a[i]), epsilon), "SIMD length() wasn't equal to reference length() at index" << i << ". Was " << lengthsA.m256_f32[i] << " but expected " << length(__a[i]) << std::endl);
        assert_true(float_equal(lengthsB.m256_f32[i], length(__b[i]), epsilon), "SIMD length() wasn't equal to reference length() at index" << i << ". Was " << lengthsB.m256_f32[i] << " but expected " << length(__b[i]) << std::endl);
    }
    std::cout << "OK!" << std::endl;
    // --------------- //
    // -------------------------------------------------------------------- //
    // -------------------------------------------------------------------- //
    // --------------- //
    std::cout << "Testing SIMD normalize... ";
    __m256Vector normalizedA = _mm256_normalize(__a);
    __m256Vector normalizedB = _mm256_normalize(__b);

    for (int i = 0; i < 8; i++) {
        assert_true(vector_equal(normalizedA[i], normalize(__a[i]), epsilon), "SIMD normalize() wasn't equal to reference normalize() at index " << i << ". Was " << normalizedA[i] << " but expected " << normalize(__a[i]) << std::endl);
        assert_true(vector_equal(normalizedB[i], normalize(__b[i]), epsilon), "SIMD normalize() wasn't equal to reference normalize() at index " << i << ". Was " << normalizedB[i] << " but expected " << normalize(__b[i]) << std::endl);
    }
    std::cout << "OK!" << std::endl;
    // --------------- //
    // -------------------------------------------------------------------- //
    // -------------------------------------------------------------------- //
    // --------------- //
    std::cout << "Testing SIMD Dot Products... ";
    __m256 dotProd = _mm256_dot_product(__a, __b);
    for (int i = 0; i < 8; i++) {
        assert_true(float_equal(dotProd.m256_f32[i], dot(__a[i], __b[i]), epsilon), "SIMD Dot Product wasn't equal to reference dot product at index " << i << ". Was " << dotProd.m256_f32[i] << " but expected " << dot(__a[i], __b[i]) << std::endl);
    }
    std::cout << "OK!" << std::endl;
    // --------------- //
    // -------------------------------------------------------------------- //
    // -------------------------------------------------------------------- //
    // --------------- //
    std::cout << "Testing SIMD Cross Products... ";
    __m256Vector crossProd = _mm256_cross_product(__a, __b);
    for (int i = 0; i < 8; i++) {
        assert_true(vector_equal(crossProd[i], cross(__a[i], __b[i]), epsilon), "SIMD Dot Product wasn't equal to reference dot product at index " << i << ". Was " << crossProd[i] << " but expected " << cross(__a[i], __b[i]) << std::endl);
    };
    std::cout << "OK!" << std::endl << std::endl;
    // --------------- //
    // -------------------------------------------------------------------- //
    // -------------------------------------------------------------------- //
}

int main()
{
    //-------------------------------------------------------------
    barycentric_coordinates_tests();
    //-------------------------------------------------------------
    //triangle_intersections_tests();
    //-------------------------------------------------------------
    SIMD_implementations_tests();
    //-------------------------------------------------------------
}
