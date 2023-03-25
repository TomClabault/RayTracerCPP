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

    //RenderSettings render_settings = RenderSettings::DEFAULT_SETTINGS;
    RenderSettings render_settings = RenderSettings::RAYTRACE_SETTINGS;
    Renderer renderer(scene, triangles, render_settings);

    totalTime += render(renderer);
    totalTime += writeImage(renderer, "image.png");





    std::cout << "\nTotal time: " << totalTime << "ms\n";
    
    return 0;
}
