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
    MeshIOData meshData;
    std::vector<Triangle> triangles;

    //meshData = read_meshio_data("data/geometry.obj");
    //triangles = MeshIOUtils::create_triangles(meshData, Translation(Vector(-1, -2, -7)) * RotationY(160) * Scale(0.02));

    meshData = read_meshio_data("data/robot.obj");
    triangles = MeshIOUtils::create_triangles(meshData, Translation(Vector(0, -2, -4)));
   
    //meshData = read_meshio_data("data/sphere_low.obj");
    //meshData = read_meshio_data("data/sphere_fat.obj");
    //triangles = MeshIOUtils::create_triangles(meshData, Translation(Vector(0, 0, 0)));

    //meshData = read_meshio_data("data/bottom_green_with_color.obj");
    //triangles = MeshIOUtils::create_triangles(meshData, Translation(Vector(1, -1, -4)));

    //Triangle triangle = Triangle(Vector(-1, 0, -2), Vector(1, 0, -2), Vector(0, 1, -2));
    //triangles.push_back(triangle);

    Scene scene(Camera(Vector(0, 0, 0)), triangles, meshData.materials, PointLight(Vector(2, 0, 2)));

    Timer timer;
    Renderer renderer(IMAGE_WIDTH, IMAGE_HEIGHT, scene);

    timer.start();
#if HYBRID_RASTERIZATION_TRACING
    renderer.rasterTrace();
#else
    renderer.trace();
#endif
    timer.stop();
    
    std::cout << timer.elapsed() << "ms\n";
    
    // enregistre l'image, de plusieurs manieres...
    write_image_png(*renderer.getImage(), "image.png");
    //write_image_bmp(*renderer.getImage(), "image.bmp");
    //write_image_hdr(*renderer.getImage(), "image.hdr");
    
    return 0;
}

