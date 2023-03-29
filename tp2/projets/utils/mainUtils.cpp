#include "image_io.h"
#include "meshIOUtils.h"
#include "renderer.h"
#include "timer.h"

float render(Renderer& renderer)
{
    Timer timer;
    
    std::cout << renderer.render_settings();
    timer.start();
    if (renderer.render_settings().hybrid_rasterization_tracing)
        renderer.raster_trace();
    else
        renderer.ray_trace();
    renderer.post_process();
    timer.stop();
    std::cout << ": " << timer.elapsed() << "ms\n\n";

    return timer.elapsed();
}

float writeImage(Renderer& renderer, const char* filepath)
{
    Timer timer;

    Image image_to_write;
    // enregistre l'image, de plusieurs manieres...
    timer.start();
    if (renderer.render_settings().enable_ssaa)
        downscale_image(*renderer.getImage(), image_to_write, renderer.render_settings().ssaa_factor);
    else
        image_to_write = *renderer.getImage();
    write_image_png(image_to_write, filepath);
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
    ///meshData = read_meshio_data("data/geometry.obj");
    //triangles = MeshIOUtils::create_triangles(meshData, Translation(Vector(-1, -3, -13)) * RotationY(160) * Scale(0.02f));

    //meshData = read_meshio_data("data/test1.obj");
    //triangles = MeshIOUtils::create_triangles(meshData, Translation(-2, -1, -3) * RotationY(45));
    //triangles = MeshIOUtils::create_triangles(meshData, Translation(-0.5, 0, -3) * RotationX(90));

    meshData = read_meshio_data("data/robot.obj");
    triangles = MeshIOUtils::create_triangles(meshData, Translation(Vector(0, -2, -4)));

    //meshData = read_meshio_data("data/burger_tom300.obj");
    //triangles = MeshIOUtils::create_triangles(meshData, Translation(Vector(0, -2, -6)));

    //meshData = read_meshio_data("data/xyzrgb_dragon.obj");
    //triangles = MeshIOUtils::create_triangles(meshData, Translation(0.25, 0, -3) * RotationY(22.5 + 180) * RotationX(90) * Scale(1.3, 1.3, 1.3));

    //meshData = read_meshio_data("data/stanford_bunny.obj");
    //triangles = MeshIOUtils::create_triangles(meshData, Translation(0, -2, -3) * RotationX(90));

    //triangles.push_back(Triangle(Point(-1, 0, -2), Point(3, 0, -2), Point(0, 1, -2)) + Vector(0, 0, -1));
    //triangles.push_back(Triangle(Point(-1, 0, -2), Point(0, 1, -2), (Point(-1, 0, -2) + Point(0, 1, -2)) / 2 + Point(-0.2, 0.5, 1)) + Vector(0, 0, -1));

    //TODO debug ces deux triangles qui donnent un edge commun noir en rasterization
    /*triangles.push_back(Triangle(Point(-1, 0, -2), Point(1, 0, -2), Point(0, 1, -2)) + Vector(0, 0, -1));
    triangles.push_back(Triangle(Point(-1, 0, -2), Point(0, 1, -2), (Point(-1, 0, -2) + Point(0, 1, -2)) / 2 + Point(-0.5, 0.5, 1)) + Vector(0, 0, -1));*/
    timer.stop();

    meshData.indices.clear();
    meshData.normals.clear();
    meshData.positions.clear();
    meshData.texcoords.clear();

    std::cout << triangles.size() << " triangles\n";
    std::cout << "\nOBJ Loading time: " << timer.elapsed() << "ms\n";

    return timer.elapsed();
}

void precompute_materials(Materials& materials)
{
    for (Material& material : materials.materials)
    {
        float luminance = 0.2126f * material.specular.r + 0.7152f * material.specular.g + 0.0722 * material.specular.b;
        float tau = std::pow(Material::SPECULAR_THRESHOLD_EPSILON / luminance, 1 / material.ns);

        material.specular_threshold = tau;
    }
}
