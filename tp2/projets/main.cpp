#include "utils/mainUtils.h"
#include "renderer.h"
#include "scene/scene.h"
#include "timer.h"

int main()
{
    float totalTime = 0.0;

    Timer timer;
    MeshIOData meshData;
    std::vector<Triangle> triangles;




    
    totalTime += loadOBJ(meshData, triangles);
    precompute_materials(meshData.materials);

    Scene scene(Camera(Point(0, 0, 0), 90), meshData.materials, PointLight(Point(2, 0, 2)));

    //RenderSettings render_settings = RenderSettings::ssaa_settings(3840, 2160, 4);
    RenderSettings render_settings = RenderSettings::basic_settings(3840, 2160);

    render_settings.use_shading = true;
    Renderer renderer(scene, triangles, render_settings);

    getchar();
    for (int i = 0; i < 50; i++)
    totalTime += render(renderer);
    totalTime += writeImage(renderer, "image.png");





    std::cout << "\nTotal time: " << totalTime << "ms\n";
    
    return 0;
}
