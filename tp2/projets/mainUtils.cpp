#include "image_io.h"
#include "meshIOUtils.h"
#include "renderer.h"
#include "timer.h"

float render(Renderer& renderer)
{
    Timer timer;

    timer.start();
#if HYBRID_RASTERIZATION_TRACING
    //getchar();
    renderer.rasterTrace();
#else
    renderer.rayTrace();
#endif
    timer.stop();
    std::cout << "Render time: " << timer.elapsed() << "ms\n";

    return timer.elapsed();
}

float writeImage(Renderer& renderer, const char* filepath)
{
    Timer timer;

    // enregistre l'image, de plusieurs manieres...
    timer.start();
    write_image_png(*renderer.getImage(), filepath);
    timer.stop();
    std::cout << "Image writing time: " << timer.elapsed() << "ms\n";
    //write_image_bmp(*renderer.getImage(), "image.bmp");
    //write_image_hdr(*renderer.getImage(), "image.hdr");

    return timer.elapsed();
}

float loadOBJ(MeshIOData& meshData, std::vector<Triangle>& triangles)
{
    Timer timer;

    timer.start();
    //meshData = read_meshio_data("data/geometry.obj");
    //triangles = MeshIOUtils::create_triangles(meshData, Translation(Vector(-1, -2, -7)) * RotationY(160) * Scale(0.02));

    meshData = read_meshio_data("data/robot.obj");
    triangles = MeshIOUtils::create_triangles(meshData, Translation(Vector(-5.5, -2, -4)));

    //meshData = read_meshio_data("data/sphere_low.obj");
    //meshData = read_meshio_data("data/sphere_fat.obj");
    //triangles = MeshIOUtils::create_triangles(meshData, Translation(Vector(0, 0, 0)));

    //meshData = read_meshio_data("data/burger01.obj");
    //triangles = MeshIOUtils::create_triangles(meshData, Translation(Vector(1, -1, -4)));

    //meshData = read_meshio_data("data/armadillo.obj");
    //triangles = MeshIOUtils::create_triangles(meshData, Translation(0, -0.8, -3) * RotationY(180) * RotationX(90));

    //meshData = read_meshio_data("data/xyzrgb_dragon.obj");
    //triangles = MeshIOUtils::create_triangles(meshData, Translation(0.25, 0, -3) * RotationY(22.5 + 180) * RotationX(90) * Scale(1.3, 1.3, 1.3));

    //meshData = read_meshio_data("data/stanford_bunny.obj");
    //triangles = MeshIOUtils::create_triangles(meshData, Translation(0, -2, -3) * RotationX(90));

    //triangles.push_back(Triangle(Vector(-1, 0, -2), Vector(1, 0, -2), Vector(0, 1, -2)));
    //triangles.push_back(Triangle(Vector(-1, -1, -2), Vector(0, -1, -2), Vector(-0.5, 1, -2)) + Vector(-3, 0, 0));
    //triangles.push_back(Triangle(Vector(-6.38484, -0.240091, -3.76875), Vector(-6.47219, -0.554191, -3.50085), Vector(-6.4422, -0.662066, -3.60034)));//Triangle of the robot not getting clipped properly
    timer.stop();

    std::cout << "\nOBJ Loading time: " << timer.elapsed() << "ms\n";

    return timer.elapsed();
}

void precompute_materials(Materials& materials)
{
    for (Material& material : materials.materials)
    {
        float luminance = 0.2126f * material.specular.r + 0.7152f * material.specular.g + 0.0722 * material.specular.b;//TODO precompute treshold in the material object
        float tau = std::powf(Material::SPECULAR_THRESHOLD_EPSILON / luminance, 1 / material.ns);//TODO epsilon en constante somewhere

        material.specular_threshold = tau;
    }
}