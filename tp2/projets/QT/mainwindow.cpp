#include "graphicsViewZoom.h"
#include "image_io.h"
#include "mainwindow.h"
#include "meshIOUtils.h"
#include "qtUtils.h"
#include "timer.h"

#include "ui_mainwindow.h"

#include <iostream>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    int width = safe_text_to_int(ui->render_width_edit->text());
    if (width == -1)
        width = _renderer.render_settings().image_width;

    int height = safe_text_to_int(ui->render_height_edit->text());
    if (height == -1)
        height = _renderer.render_settings().image_height;

    _renderer.change_render_size(width, height);
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
    this->graphics_view_zoom = new Graphics_view_zoom(ui->graphics_view);
    this->ui->graphics_view->setScene(scene);
}

void MainWindow::prepare_renderer_buffers()
{
    bool new_ssaa;
    int new_ssaa_factor;
    int new_width = safe_text_to_int(ui->render_width_edit->text());
    int new_height = safe_text_to_int(ui->render_height_edit->text());

    if (new_width == -1)
        new_width = _renderer.render_settings().image_width;
    if (new_height == -1)
        new_height = _renderer.render_settings().image_height;
    new_ssaa = this->ui->enable_ssaa_check_box->isChecked();
    new_ssaa_factor = this->ui->ssaa_spin_box->value();

    bool render_size_changed = (new_width != _renderer.render_settings().image_width)   ||
                               (new_height != _renderer.render_settings().image_height) ||
                               (new_ssaa != _renderer.render_settings().enable_ssaa)    ||
                               (new_ssaa_factor != _renderer.render_settings().ssaa_factor);

    //Recreating the buffers if the width or the height changed
    if (render_size_changed)
    {
        _renderer.render_settings().enable_ssaa = new_ssaa;
        _renderer.render_settings().ssaa_factor = new_ssaa_factor;

        _renderer.change_render_size(new_width, new_height);
    }
    else//If the buffers haven't been recreated, we're just clearing them
    {
        _renderer.clear_z_buffer();
        _renderer.clear_image();
    }
}

void MainWindow::on_renderButton_clicked()
{
    prepare_renderer_buffers();

    if (_renderer.render_settings().hybrid_rasterization_tracing)
        _renderer.raster_trace();
    else
        _renderer.ray_trace();

    if (_rendererd_image_allocated)
    {
        delete _renderered_image;

        _rendererd_image_allocated = false;
    }

    if (_renderer.render_settings().enable_ssaa)
    {
        downscale_image(*_renderer.getImage(), &_renderered_image, _renderer.render_settings().ssaa_factor);
        _rendererd_image_allocated = true;
    }
    else
        _renderered_image = _renderer.getImage();

    set_render_image(_renderered_image);
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

void MainWindow::on_load_robot_obj_button_clicked()
{
    load_obj("data/robot.obj");
}

void MainWindow::on_hybrid_check_box_stateChanged(int value)
{
    _renderer.render_settings().hybrid_rasterization_tracing = value;
}

void MainWindow::on_clipping_check_box_stateChanged(int value)
{
    _renderer.render_settings().enable_clipping = value;
}

void MainWindow::on_enable_ssaa_check_box_stateChanged(int value)
{
    if (value)
        this->ui->ssaa_spin_box->setEnabled(true);
    else
        this->ui->ssaa_spin_box->setEnabled(false);
}

void MainWindow::on_dump_render_to_file_button_clicked()
{
    QString filename = this->ui->dump_to_file_filename_edit->text();
    if (filename == "")
        return;

    this->q_image->save(filename);
}

void MainWindow::on_camera_fov_spin_box_valueChanged(int fov)
{
    _renderer.change_camera_fov(fov);
}
