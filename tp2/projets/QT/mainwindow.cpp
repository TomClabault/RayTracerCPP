#include "graphicsViewZoom.h"
#include "image_io.h"
#include "mainwindow.h"
#include "meshIOUtils.h"
#include "qtUtils.h"
#include "timer.h"

#include "./ui_mainwindow.h"

#include <iostream>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::set_render_image(const Image* const image)
{
    QGraphicsScene* scene = new QGraphicsScene(this);

    if (this->q_image != nullptr)
        delete this->q_image;
    this->q_image = new QImage((uchar*)image->data(), image->width(), image->height(), QImage::Format_RGBA32FPx4);
    this->q_image->mirror();//Because our ray tracer produces flipped images
    scene->addPixmap(QPixmap::fromImage(*this->q_image));

    if (this->graphics_view_zoom != nullptr)
        delete this->graphics_view_zoom;
    this->graphics_view_zoom = new Graphics_view_zoom(ui->graphicsView_2);
    this->ui->graphicsView_2->setScene(scene);
}

void MainWindow::on_renderButton_clicked()
{
    if (_renderer.render_settings().hybrid_rasterization_tracing)
        _renderer.raster_trace();
    else
        _renderer.ray_trace();

    Image image_to_write;
    if (_renderer.render_settings().enable_ssaa)
        downscale_image(*_renderer.getImage(), *image_to_write, _renderer.render_settings().ssaa_factor);
    else
        image_to_write = _renderer.getImage();

    set_render_image(image_to_write);
}

void MainWindow::load_obj(const char* filepath)
{
    Timer timer;

    timer.start();

    MeshIOData meshData = read_meshio_data("data/robot.obj");
    std::vector<Triangle> triangles = MeshIOUtils::create_triangles(meshData, Translation(Vector(0, -2, -4)));

    _renderer.set_triangles(triangles);
    _renderer.set_materials(meshData.materials);

    timer.stop();

    std::cout << "OBJ Loading time: " << timer.elapsed() << "ms" << std::endl;
}

void MainWindow::on_loadRobotObjButton_clicked()
{
    load_obj("data/robot.obj");
}

void MainWindow::on_hybrid_check_box_stateChanged(int value)
{
    _renderer.render_settings().hybrid_rasterization_tracing = value;
}

void MainWindow::on_render_wdith_edit_editingFinished()
{
    int width = safe_text_to_int(ui->render_wdith_edit->text());

    if (width != -1)
        _renderer.render_settings().image_width = width;
}

void MainWindow::on_render_height_edit_editingFinished()
{
    int height = safe_text_to_int(ui->render_wdith_edit->text());

    if (height != -1)
        _renderer.render_settings().image_height = height;
}

void MainWindow::on_clipping_check_box_stateChanged(int value)
{
    _renderer.render_settings().enable_clipping = value;
}

void MainWindow::on_enable_ssaa_check_box_stateChanged(int value)
{
    if (value)
    {
        _renderer.render_settings().enable_ssaa = true;
        _renderer.render_settings().ssaa_factor = this->ui->ssaa_spin_box->value();

        this->ui->ssaa_spin_box->setEnabled(true);
    }
    else
    {
        _renderer.render_settings().enable_ssaa = false;

        this->ui->ssaa_spin_box->setEnabled(false);
    }
}

void MainWindow::on_dump_render_to_file_button_clicked()
{
    QString filename = this->ui->dump_to_file_filename_edit->text();
    if (filename == "")
        return;

    this->q_image->save(filename);
}

