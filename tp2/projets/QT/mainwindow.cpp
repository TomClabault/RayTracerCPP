#include "graphicsViewZoom.h"
#include "image_io.h"
#include "mainwindow.h"
#include "mainUtils.h"
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

    //Updating the renderer setting with the values present in the fields
    //at the time the UI is built. This is to make sure that the UI is coherent
    //with the actual settings of the renderer
    on_ssao_1_amount_edit_editingFinished();
    on_ssao_1_radius_edit_editingFinished();
    on_ssao_1_sample_count_edit_editingFinished();

    on_camera_fov_spin_box_valueChanged(this->ui->camera_fov_spin_box->value());

    //Initializing the random generator here for later uses
    srand(time(NULL));
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

    QImage::Format format;
#if QT_VERSION >= 0x060000
    format = QImage::Format_RGBA32FPx4;
#else
    format = QImage::Format_RGB32;
#endif

    this->q_image = new QImage((uchar*)image->data(), image->width(), image->height(), format);

    //Flipping the image because our ray tracer produces flipped images
#if QT_VERSION >= 0x060000
    this->q_image->mirror();
#else
    QImage mirrored = this->q_image->mirrored();
    delete q_image;
    q_image = new QImage(mirrored);
#endif
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
    new_ssaa = this->ui->ssaa_radio_button->isChecked();
    new_ssaa_factor = safe_text_to_int(this->ui->ssaa_factor_edit->text());
    if (new_ssaa_factor == -1)
        new_ssaa_factor = _renderer.render_settings().ssaa_factor;

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

    //Clearing the buffers
    _renderer.clear_z_buffer();
    _renderer.clear_normal_buffer();
    _renderer.clear_image();
}

void MainWindow::on_renderButton_clicked()
{
    prepare_renderer_buffers();

    if (_renderer.render_settings().hybrid_rasterization_tracing)
        _renderer.raster_trace();
    else
        _renderer.ray_trace();
    _renderer.post_process();

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

void MainWindow::load_obj(const char* filepath, Transform transform)
{
    Timer timer;

    timer.start();

    MeshIOData meshData = read_meshio_data(filepath);
    std::vector<Triangle> triangles = MeshIOUtils::create_triangles(meshData, transform);

    _renderer.set_triangles(triangles);
    _renderer.set_materials(meshData.materials);
    precompute_materials(_renderer.get_materials());

    timer.stop();

    std::cout << "OBJ Loading time: " << timer.elapsed() << "ms" << std::endl;
}

void MainWindow::on_load_robot_obj_button_clicked()
{
    load_obj("data/robot.obj", Translation(Vector(0, -2, -4)));
}

void MainWindow::on_load_geometry_obj_button_clicked()
{
    load_obj("data/geometry.obj", Translation(Vector(-1, -3, -12)) * RotationY(160) * Scale(0.02f));
}

void MainWindow::on_hybrid_check_box_stateChanged(int value)
{
    _renderer.render_settings().hybrid_rasterization_tracing = value;
    this->ui->clipping_check_box->setEnabled(value);
}

void MainWindow::on_clipping_check_box_stateChanged(int value)
{
    _renderer.render_settings().enable_clipping = value;
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

void MainWindow::on_render_width_edit_returnPressed()
{
    on_renderButton_clicked();
}

void MainWindow::on_render_height_edit_returnPressed()
{
    on_renderButton_clicked();
}

void MainWindow::on_enable_ssao_1_checkbox_stateChanged(int value)
{
    _renderer.render_settings().enable_ssao = value;

    this->ui->ssao_1_radius_edit->setEnabled(value);
    this->ui->ssao_1_radius_label->setEnabled(value);
    this->ui->ssao_1_sample_count_edit->setEnabled(value);
    this->ui->ssao_1_sample_count_label->setEnabled(value);
    this->ui->ssao_1_amount_edit->setEnabled(value);
    this->ui->ssao_1_amount_label->setEnabled(value);
}

void MainWindow::on_ssao_1_radius_edit_editingFinished()
{
    float radius = safe_text_to_float(this->ui->ssao_1_radius_edit->text());

    if (radius == -1)
        return;

    _renderer.render_settings().ssao_radius = radius;
}

void MainWindow::on_ssao_1_sample_count_edit_editingFinished()
{
    int sample_count = safe_text_to_int(this->ui->ssao_1_sample_count_edit->text());

    if (sample_count == -1)
        return;

    _renderer.render_settings().ssao_sample_count = sample_count;
}

void MainWindow::on_ssao_1_amount_edit_editingFinished()
{
    float amount = safe_text_to_float(this->ui->ssao_1_amount_edit->text());

    if (amount == -1)
        return;

    _renderer.render_settings().ssao_amount = amount;
}

void MainWindow::on_ssaa_radio_button_toggled(bool checked)
{
    this->ui->ssaa_factor_edit->setEnabled(checked);
    this->ui->ssaa_factor_label->setEnabled(checked);
}
