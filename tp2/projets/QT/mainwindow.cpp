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

    //Initializing the camera's FOV
    on_camera_fov_edit_editingFinished();

    //Initializing the light's position
    on_light_position_edit_editingFinished();

    //Initializing the random generator here for later uses
    srand(time(NULL));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::handle_image_processing()
{
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

void MainWindow::set_render_image(const Image* const image)
{
    QGraphicsScene* scene = new QGraphicsScene(this);

    if (this->q_image != nullptr)
        delete this->q_image;

#if QT_VERSION >= 0x060000
    this->q_image = new QImage((uchar*)image->data(), image->width(), image->height(), QImage::Format_RGBA32FPx4);
#else
    //Because Qt5 doesn't have the QImage::Format_RGBA32FPx4 format which our image is encoded in
    //we're converting our image to a Format_RGBA8888 which Qt5 supports
    uchar* image_8888_data = new uchar[image->width() * image->height() * 4];
    for (int i = 0; i < image->height(); i++)
    {
        for (int j = 0; j < image->width(); j++)
        {
            uchar r, g, b, a;
            r = (uchar)(std::clamp(image->operator()(j, i).r, 0.0f, 1.0f) * 255);
            g = (uchar)(std::clamp(image->operator()(j, i).g, 0.0f, 1.0f) * 255);
            b = (uchar)(std::clamp(image->operator()(j, i).b, 0.0f, 1.0f) * 255);
            a = (uchar)(std::clamp(image->operator()(j, i).a, 0.0f, 1.0f) * 255);

            image_8888_data[(i * image->width() + j) * 4 + 0] = r;
            image_8888_data[(i * image->width() + j) * 4 + 1] = g;
            image_8888_data[(i * image->width() + j) * 4 + 2] = b;
            image_8888_data[(i * image->width() + j) * 4 + 3] = a;
        }
    }

    this->q_image = new QImage(image_8888_data, image->width(), image->height(), QImage::Format_RGBA8888);
#endif

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

void MainWindow::prepare_bvh()
{
    bool new_bvh_enabled = this->ui->enable_bvh_check_box->isChecked();
    int new_bvh_max_depth = safe_text_to_int(this->ui->bvh_max_depth_edit->text());
    if (new_bvh_max_depth == -1)//An invalid value was entered in the input field
        new_bvh_max_depth = _renderer.render_settings().bvh_max_depth;
    int new_bvh_max_obj_count = safe_text_to_int(this->ui->bvh_max_leaf_object_edit->text());
    if (new_bvh_max_obj_count == -1)
        new_bvh_max_obj_count = _renderer.render_settings().bvh_leaf_object_count;


    //The user just enabled the BVH or changed the settings of the BVH
    if ((new_bvh_enabled && !_renderer.render_settings().enable_bvh) ||
        (new_bvh_max_depth != _renderer.render_settings().bvh_max_depth ||
         new_bvh_max_obj_count != _renderer.render_settings().bvh_leaf_object_count))
    {
        _renderer.render_settings().bvh_max_depth = new_bvh_max_depth;
        _renderer.render_settings().bvh_leaf_object_count = new_bvh_max_obj_count;

        _renderer.reconstruct_bvh_new();
    }
    else if (!new_bvh_enabled && _renderer.render_settings().enable_bvh)
        _renderer.destroy_bvh(); //The user just disabled the BVH

    _renderer.render_settings().bvh_max_depth = new_bvh_max_depth;
    _renderer.render_settings().bvh_leaf_object_count = new_bvh_max_obj_count;
    _renderer.render_settings().enable_bvh = new_bvh_enabled;
}

void MainWindow::prepare_renderer_buffers()
{
    bool new_ssaa, new_ssao;
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
    new_ssao = this->ui->enable_ssao_1_checkbox->isChecked();

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

    if (new_ssao != _renderer.render_settings().enable_ssao)
    {
        _renderer.render_settings().enable_ssao = new_ssao;

        if (new_ssao)
            _renderer.prepare_ssao_buffers();
        else
            _renderer.destroy_ssao_buffers();
    }

    //Clearing the buffers
    _renderer.clear_z_buffer();
    _renderer.clear_normal_buffer();
    _renderer.clear_image();
}

void MainWindow::on_render_button_clicked()
{
    std::stringstream ss;

    Timer timer;
    prepare_bvh();//Updates the BVH if necessary
    timer.start();
    prepare_renderer_buffers();//Updates the various buffers of the renderer
    //if necessary
    timer.stop();

    ss << std::endl << "Buffer intialization time: " << timer.elapsed() << "ms" << std::endl;

    timer.start();
    if (_renderer.render_settings().hybrid_rasterization_tracing)
        _renderer.raster_trace();
    else
        _renderer.ray_trace();
    timer.stop();
    ss << "Render time: " << timer.elapsed() << "ms" << std::endl;

    timer.start();
    _renderer.post_process();
    timer.stop();
    ss << "Post-processing time: " << timer.elapsed() << "ms";

    //Allocates the image buffer, downscales the image if SSAA was used, ....
    //and finally display the rendered image in the QGraphicsView
    handle_image_processing();

    write_to_console(ss);
}

void MainWindow::load_obj(const char* filepath, Transform transform)
{
    std::stringstream ss;
    Timer timer;

    timer.start();

    MeshIOData meshData = read_meshio_data(filepath);
    //std::vector<Triangle> triangles = MeshIOUtils::create_triangles(meshData, transform);
    std::vector<Triangle> triangles;
    triangles.push_back(Triangle(Point(0.032414,0.763173,-5.07584), Point(-0.055944,0.887663,-2.90412), Point(-0.579099,0.855624,-3.02897)));

    _renderer.set_triangles(triangles);
    //_renderer.set_materials(meshData.materials);
    //precompute_materials(_renderer.get_materials());


    timer.stop();

    ss << "OBJ Loading time: " << timer.elapsed() << "ms";
    write_to_console(ss);
}

void MainWindow::write_to_console(const std::stringstream& ss)
{
    std::string ss_string = ss.str();

    this->ui->output_console->append(QString(ss_string.c_str()));
    this->ui->output_console->ensureCursorVisible();
}

void MainWindow::on_load_robot_obj_button_clicked()
{
    load_obj("./data/robot.obj", Translation(Vector(0, -2, -4)));
}

void MainWindow::on_load_geometry_obj_button_clicked()
{
    load_obj("./data/geometry.obj", Translation(Vector(-1, -3, -12)) * RotationY(160) * Scale(0.02f));
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

void MainWindow::on_render_width_edit_returnPressed() { on_render_button_clicked(); }
void MainWindow::on_render_height_edit_returnPressed() { on_render_button_clicked(); }

void MainWindow::on_enable_ssao_1_checkbox_stateChanged(int value)
{
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

void MainWindow::on_ssaa_factor_edit_returnPressed() { on_render_button_clicked(); }

void MainWindow::on_ssao_1_sample_count_edit_returnPressed() { on_render_button_clicked(); }
void MainWindow::on_ssao_1_radius_edit_returnPressed() { on_render_button_clicked(); }
void MainWindow::on_ssao_1_amount_edit_returnPressed() { on_render_button_clicked(); }

void MainWindow::on_camera_fov_edit_editingFinished()
{
    float fov = safe_text_to_float(this->ui->camera_fov_edit->text());
    if (fov != -1)
        _renderer.change_camera_fov(fov);
}


void MainWindow::on_camera_fov_edit_returnPressed() { on_render_button_clicked(); }

void MainWindow::on_bvh_max_leaf_object_edit_returnPressed() { on_render_button_clicked(); }
void MainWindow::on_bvh_max_depth_edit_returnPressed() { on_render_button_clicked(); }

void MainWindow::on_enable_bvh_check_box_stateChanged(int checked)
{
    this->ui->bvh_max_depth_edit->setEnabled(checked);
    this->ui->bvh_max_depth_label->setEnabled(checked);
    this->ui->bvh_max_leaf_object_edit->setEnabled(checked);
    this->ui->bvh_max_leaf_object_label->setEnabled(checked);
}

void MainWindow::on_enable_shadows_check_box_stateChanged(int checked) { _renderer.render_settings().compute_shadows = checked; }

void MainWindow::on_light_position_edit_editingFinished()
{
    QString light_position_text = this->ui->light_position_edit->text();
    QStringList coordinates = light_position_text.split("/");
    if (coordinates.size() != 3)
        return;

    bool ok_x, ok_y, ok_z;
    float x = safe_text_to_float(coordinates.at(0), ok_x);
    float y = safe_text_to_float(coordinates.at(1), ok_y);
    float z = safe_text_to_float(coordinates.at(2), ok_z);

    if (!ok_x || !ok_y || !ok_z)
        return;
    else
        _renderer.set_light_position(Point(x, y, z));
}

void MainWindow::on_light_position_edit_returnPressed() { on_light_position_edit_editingFinished(); on_render_button_clicked(); }

//TODO profiler l'imple SIMD de la SSAO pour voir si y'a des bottleneck
