#ifndef BENCHMARK_H
#define BENCHMARK_H

#include "mat.h"

class Benchmark
{
public:
	static void benchmark_bvh_parameters(const char* filepath, Transform model_transform, int min_obj_count, int max_obj_count, int min_depth, int max_depth, int iterations, int obj_count_step = 1, int depth_step = 1);
};

#endif