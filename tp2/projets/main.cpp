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

    std::cout << "\nRender resolution: " << IMAGE_WIDTH << "*" << IMAGE_HEIGHT;
#if ENABLE_SSAA
    Renderer renderer(IMAGE_WIDTH * SSAA_FACTOR, IMAGE_HEIGHT * SSAA_FACTOR, scene);
    std::cout << " SSAAx" << SSAA_FACTOR;
#else
    Renderer renderer(IMAGE_WIDTH, IMAGE_HEIGHT, scene);
#endif
    std::cout << "\n";

    //TODO faire une structure à passer au renderer render() pour garder toutes les settings du rendu. Comme ça on peut lancer plusieurs rendus d'affilée avec des settings différentes.
    for (int i = 0; i < 20; i++)
    totalTime += render(renderer);
    totalTime += writeImage(renderer, "image.png");





    std::cout << "\nTotal time: " << totalTime << "ms\n";
    
    return 0;
}
