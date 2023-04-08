#include "benchmark.h"
#include "mainUtils.h"
#include "renderer.h"

void Benchmark::benchmark_bvh_parameters(const char* filepath, Transform model_transform, int min_obj_count, int max_obj_count, int min_depth, int max_depth, int iterations, int obj_count_step, int depth_step)
{
	MeshIOData mesh_data = read_meshio_data(filepath);
	std::vector<Triangle> triangles = MeshIOUtils::create_triangles(mesh_data, model_transform);

	Scene scene = Scene(Camera(Point(0, 0, 0), 90), mesh_data.materials, PointLight(Point(2, 0, 2)));//TODO mettre les Materials dans le renderer

	int best_obj_count;
	int best_depth;
	float best_timing = INFINITY;
	for (int obj_count = min_obj_count; obj_count <= max_obj_count; obj_count += obj_count_step)
	{
		for (int depth = min_depth; depth <= max_depth; depth += depth_step)
		{
			RenderSettings render_settings = RenderSettings::basic_settings(1920, 1080, false);
			render_settings.bvh_leaf_object_count = obj_count;
            render_settings.bvh_max_depth = depth;
            render_settings.shading_method = RenderSettings::ShadingMethod::RT_SHADING;

			Renderer renderer(scene, triangles, render_settings);

			float local_timing = INFINITY;
			for (int i = 0; i < iterations; i++)
			{
				float timing = render(renderer);
				if (timing < local_timing)
				{
					local_timing = timing;

					if (best_timing > timing)
					{
						best_timing = timing;
						best_depth = depth;
						best_obj_count = obj_count;
					}
				}
			}

		}
	}

	std::cout << "Best settings found for model [" << filepath << "]: [obj_count, max_depth]=[" << best_obj_count << ", " << best_depth << "] with " << best_timing << "ms\n";
}
