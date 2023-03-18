#include <algorithm>
#include <array>
#include <execution>
#include <numeric>

#include "image.h"
#include "image_io.h"
#include "mesh_io.h"
#include "meshIOUtils.h"
#include "objUtils.h"
#include "renderer.h"
#include "scene.h"
#include "settings.h"
#include "timer.h"
#include "triangle.h"
#include "mat.h"

int main()
{
    float totalTime = 0.0;

    Timer timer;
    MeshIOData meshData;
    std::vector<Triangle> triangles;

    timer.start();
    //meshData = read_meshio_data("data/geometry.obj");
    //triangles = MeshIOUtils::create_triangles(meshData, Translation(Vector(-1, -2, -7)) * RotationY(160) * Scale(0.02));

    //meshData = read_meshio_data("data/robot.obj");
    //triangles = MeshIOUtils::create_triangles(meshData, Translation(Vector(0, -2, -4)));
   
    //meshData = read_meshio_data("data/sphere_low.obj");
    //meshData = read_meshio_data("data/sphere_fat.obj");
    //triangles = MeshIOUtils::create_triangles(meshData, Translation(Vector(0, 0, 0)));

    //meshData = read_meshio_data("data/burger01.obj");
    //triangles = MeshIOUtils::create_triangles(meshData, Translation(Vector(1, -1, -4)));

    //meshData = read_meshio_data("data/armadillo.obj");
    //triangles = MeshIOUtils::create_triangles(meshData, Translation(0, -0.8, -3) * RotationY(180) * RotationX(90));

    //meshData = read_meshio_data("data/xyzrgb_dragon.obj");
    //triangles = MeshIOUtils::create_triangles(meshData, Translation(0.25, 0, -3) * RotationY(22.5 + 180) * RotationX(90) * Scale(1.3, 1.3, 1.3));

    meshData = read_meshio_data("data/stanford_bunny.obj");
    triangles = MeshIOUtils::create_triangles(meshData, Translation(0, -2, -3) * RotationX(90));

    //triangles.push_back(Triangle(Vector(-1, 0, -2), Vector(1, 0, -2), Vector(0, 1, -2)));
     //triangles.push_back(Triangle(Vector(-1, 0, -2), Vector(1, -0.5, -3), Vector(1, 0, -3)) + Vector(0, 1, 0));
    //triangles.push_back(Triangle(Vector(-1, -1, -1), Vector(0, -1, -1), Vector(-1, 0, -1)) + Vector(-0.2, 0, 0));
    //triangles.push_back(Triangle(Vector(-1, 0, 2), Vector(1, 0, 2), Vector(0, 1, 2)));
    //triangles.push_back(Triangle(Vector(-1, 0, 2), Vector(1, 0, -2), Vector(0, 1, -2)));
    //triangles.push_back(Triangle(Vector(-0.5, -0.5, -2), Vector(0, -0.5, 3), Vector(0.5, -0.5, -2)));
    timer.stop();
    totalTime += timer.elapsed();

    std::cout << "OBJ Loading time: " << timer.elapsed() << "ms\n";

    Scene scene(Camera(Vector(0, 0, 0)), triangles, meshData.materials, PointLight(Vector(2, 0, 2)));

    Renderer renderer(IMAGE_WIDTH, IMAGE_HEIGHT, scene);

    std::cout << "\nRender resolution: " << IMAGE_WIDTH << "*" << IMAGE_HEIGHT << "\n";
    timer.start();
#if HYBRID_RASTERIZATION_TRACING
    renderer.rasterTrace();
#else
    renderer.rayTrace();
#endif
    timer.stop();
    totalTime += timer.elapsed();
    std::cout << "Render time: " << timer.elapsed() << "ms\n";
    
    
    // enregistre l'image, de plusieurs manieres...
    timer.start();
    write_image_png(*renderer.getImage(), "image.png");
    timer.stop();
    totalTime += timer.elapsed();
    std::cout << "Image writing time: " << timer.elapsed() << "ms\n";
    //write_image_bmp(*renderer.getImage(), "image.bmp");
    //write_image_hdr(*renderer.getImage(), "image.hdr");

    std::cout << "\nTotal time: " << totalTime << "ms\n";
    
    return 0;
}

