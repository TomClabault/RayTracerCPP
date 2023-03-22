#include "mainUtils.h"
#include "renderer.h"
#include "scene.h"
#include "timer.h"

int main()
{
    float totalTime = 0.0;

    Timer timer;
    MeshIOData meshData;
    std::vector<Triangle> triangles;




    
    totalTime += loadOBJ(meshData, triangles);
    precompute_materials(meshData.materials);

    Scene scene(Camera(Point(0, 0, 0), 90), triangles, meshData.materials, PointLight(Point(2, 0, 2)));
    Renderer renderer(IMAGE_WIDTH, IMAGE_HEIGHT, scene);
    std::cout << "\nRender resolution: " << IMAGE_WIDTH << "*" << IMAGE_HEIGHT << "\n";

    totalTime += render(renderer);
    totalTime += writeImage(renderer, "image.png");





    std::cout << "\nTotal time: " << totalTime << "ms\n";
    
    return 0;
}
