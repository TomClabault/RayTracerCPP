#include "utils/mainUtils.h"
#include "renderer.h"
#include "scene/scene.h"
#include "timer.h"

#include <omp.h>

//TODO free les octree node à la destruction de la BVH
//TODO mettre les fonctions de mainUtils pour qu'elles renvoient des long lng int plutpot que des floats: les milliseoncdes sont pas en float
#define MIN 8
#define MAX 20//TODO faire un benchmark de la BVH plutôt que de mettre ça dans le main
#define ITER 50

/*int bestTimes[MAX - MIN + 1];
    for (int i = MIN; i <= MAX; i++)
    {
        RenderSettings render_settings = RenderSettings::basic_settings(1920 / 2, 1080 / 2);

        render_settings.use_shading = true;
        render_settings.hybrid_rasterization_tracing = false;
        render_settings.bvh_max_depth = i;
        Renderer renderer(scene, triangles, render_settings);

        long long int bestTiming = 1000000000;
        for (int j = 0; j < ITER; j++)
        {
            float timing = render(renderer);
            if (timing < bestTiming)
                bestTiming = timing;
        }

        bestTimes[i - MIN] = bestTiming;
        std::cout << "Best timing depth " << i << "=" << bestTiming << "\n";
    }*/

#include "benchmark.h"

int main()
{
    //Benchmark::benchmark_bvh_parameters("data/xyzrgb_dragon.obj", Translation(0.25, 0, -3) * RotationY(22.5 + 180) * RotationX(90) * Scale(1.3, 1.3, 1.3), 8, 8, 12, 12, 10);
    Benchmark::benchmark_bvh_parameters("data/robot.obj", Translation(Vector(0, -2, -4)), 10, 150, 8, 16, 20, 10);

    return 0;

    float totalTime = 0.0;

    Timer timer;
    MeshIOData meshData;
    std::vector<Triangle> triangles;


    
    totalTime += loadOBJ(meshData, triangles);
    precompute_materials(meshData.materials);

    Scene scene(Camera(Point(0, 0, 0), 90), meshData.materials, PointLight(Point(2, 0, 2)));

    //RenderSettings render_settings = RenderSettings::ssaa_settings(3840, 2160, 4);
    RenderSettings render_settings = RenderSettings::basic_settings(1920 / 2, 1080 / 2);
    render_settings.use_shading = true;
    render_settings.hybrid_rasterization_tracing = false;
    render_settings.bvh_max_depth = 12;
    Renderer renderer(scene, triangles, render_settings);

    getchar();
    for (int i = 0; i < 50; i++)
    totalTime += render(renderer);
    totalTime += writeImage(renderer, "image.png");



    std::cout << "\nTotal time: " << totalTime << "ms\n";
    
    return 0;
}
