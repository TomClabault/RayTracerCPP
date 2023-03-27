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
    Timer timer;
    MeshIOData meshData;
    std::vector<Triangle> triangles;


    
    loadOBJ(meshData, triangles);
    precompute_materials(meshData.materials);

    Scene scene(Camera(Point(0, 0, 0), 90), meshData.materials, PointLight(Point(2, 0, 2)));

    //RenderSettings render_settings = RenderSettings::ssaa_settings(3840, 2160, 4);
    RenderSettings render_settings = RenderSettings::basic_settings(1920, 1080);
    render_settings.use_shading = true;
    render_settings.hybrid_rasterization_tracing = true;
    render_settings.compute_shadows = true;
    render_settings.bvh_max_depth = 12;
    render_settings.bvh_leaf_object_count = 8;
    render_settings.enable_bvh = true;
    Renderer renderer(scene, triangles, render_settings);

    //getchar();
    //for (int i = 0; i < 50; i++)
    render(renderer);
    writeImage(renderer, "image.png");

    return 0;
}
