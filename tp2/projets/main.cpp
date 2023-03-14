#include <algorithm>
#include <array>
#include <execution>
#include <numeric>

#include "image.h"
#include "image_io.h"
#include "mesh_io.h"
#include "meshIOUtils.h"
#include "objUtils.h"
#include "rayTracer.h"
#include "scene.h"
#include "settings.h"
#include "timer.h"
#include "triangle.h"
#include "mat.h"

int main()
{
    MeshIOData meshData;
    std::vector<Triangle> triangles;

    /*meshData = read_meshio_data("data/geometry.obj");
    triangles = MeshIOUtils::createTriangles(meshData, Translation(Vector(-1, -2, -7)) * RotationY(180) * Scale(0.02));*/

    meshData = read_meshio_data("data/robot.obj");
    triangles = MeshIOUtils::createTriangles(meshData, Translation(Vector(0, -2, -4)));

    Scene scene(Camera(Vector(0, 0, 0)), triangles, meshData._materials, PointLight(Vector(-2, 0, 0)));

    Timer timer;
    RayTracer rayTracer(IMAGE_WIDTH, IMAGE_HEIGHT, scene);

    timer.start();
    rayTracer.trace();
    timer.stop();
    
    std::cout << timer.elapsed() << "ms\n";
    
    // enregistre l'image, de plusieurs manieres...
    write_image_png(*rayTracer.getImage(), "image.png");
    //write_image_bmp(*rayTracer.getImage(), "image.bmp");
    //write_image_hdr(image, "image.hdr");
    
    return 0;
}

