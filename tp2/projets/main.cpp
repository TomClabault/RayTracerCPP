//TODO mettre les fonctions de mainUtils pour qu'elles renvoient des long lng int plutpot que des floats: les milliseoncdes sont pas en float

#include "QT/mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}

//int main()
//{
//    Timer timer;
//    MeshIOData meshData;
//    std::vector<Triangle> triangles;


    
//    loadOBJ(meshData, triangles);
//    precompute_materials(meshData.materials);

//    Scene scene(Camera(Point(0, 0, 0), 90), meshData.materials, PointLight(Point(3, 3, 2)));

//    //RenderSettings render_settings = RenderSettings::ssaa_settings(3840, 2160, 4);
//    RenderSettings render_settings = RenderSettings::basic_settings(1920*2, 1080*2);
//    render_settings.use_shading = true;
//    render_settings.hybrid_rasterization_tracing = true;
//    render_settings.compute_shadows = false;
//    render_settings.bvh_max_depth = 12;
//    render_settings.bvh_leaf_object_count = 8;
//    render_settings.enable_bvh = true;
//    render_settings.enable_ssao = true;
//    Renderer renderer(scene, triangles, render_settings);

//    //getchar();
//    //for (int i = 0; i < 50; i++)
//    render(renderer);
//    writeImage(renderer, "image.png");

//    return 0;
//}
