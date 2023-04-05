#ifndef XORSHIFT_H
#define XORSHIFT_H

#include <cstdint>
#include <immintrin.h>
#include <limits>

struct __m256_XorShiftGenerator
{
    __m256_XorShiftGenerator() : _state(_mm256_set_epi32(rand(), rand(), rand(), rand(), rand(), rand(), rand(), rand())) {}
    __m256_XorShiftGenerator(__m256i state) : _state(state) {}

    __m256i get_rand()
    {
        __m256i x = _state;

        x = _mm256_xor_si256(x, _mm256_slli_epi32(x, 13));
        x = _mm256_xor_si256(x, _mm256_srli_epi32(x, 17));
        x = _mm256_xor_si256(x, _mm256_slli_epi32(x, 5));

        return _state = x;
    }

    __m256 get_rand_bilateral()
    {
        return _mm256_div_ps(_mm256_cvtepi32_ps(get_rand()), _mm256_set1_ps((float)std::numeric_limits<int32_t>::max()));
    }

    __m256 get_rand_lateral()
    {
        return _mm256_mul_ps(_mm256_add_ps(_mm256_div_ps(_mm256_cvtepi32_ps(get_rand()), _mm256_set1_ps((float)std::numeric_limits<int32_t>::max())), _mm256_set1_ps(1.0f)), _mm256_set1_ps(0.5f));
    }

    __m256i _state;
};

struct XorShiftGenerator 
{
    XorShiftGenerator() : _state(rand()) {}
    XorShiftGenerator(uint32_t state) : _state(state) {}

    uint32_t get_rand()
    {
        uint32_t x = _state;

        x ^= x << 13;
        x ^= x >> 17;
        x ^= x << 5;

        return _state = x;
    }

    float get_rand_bilateral()
    {
        return get_rand() / (float)std::numeric_limits<uint32_t>::max() * 2 - 1;
    }

    float get_rand_lateral()
    {
        return get_rand() / (float)std::numeric_limits<uint32_t>::max();
    }

    /* The state must be initialized to non-zero */
    uint32_t _state;
}; 

#endif
