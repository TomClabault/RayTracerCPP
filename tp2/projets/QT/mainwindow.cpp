#include "editMaterialDialog.h"
#include "graphicsViewZoom.h"
#include "image_io.h"
#include "mainwindow.h"
#include "meshIOUtils.h"
#include "qtUtils.h"
#include "timer.h"

#include "ui_mainwindow.h"

#include <iostream>

#include <QFileDialog>
#include <thread>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow), _display_thread_handle(this),
    _render_thread_handle(this)
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

    //Enabling the skybox by default
    _renderer.set_skybox(read_image("./data/skybox.jpg", false));
    _renderer.render_settings().enable_skybox = true;

    setup_render_display_context();
    connect(&_display_thread_handle, &DisplayThread::update_image, this, &MainWindow::update_render_image);
    connect(&_render_thread_handle, &RenderThread::update_image, this, &MainWindow::update_render_image);
    _display_thread_handle.start();

    //Initializing the random generator here for later uses
    srand(time(NULL));
}

MainWindow::~MainWindow()
{
    delete ui;

    //Clean exit of the display thread
    _display_thread_handle.quit();
    _display_thread_handle.wait();
}

void MainWindow::setup_render_display_context()
{
    //_render_display_context._graphics_scene = new QGraphicsScene(this);
    _render_display_context._graphics_view_zoom = new Graphics_view_zoom(ui->graphics_view);
}

void MainWindow::update_render_image()
{
    _render_display_context._mutex.lock();

    Timer timer;
    timer.start();

    if (_render_display_context._q_image != nullptr)
    {
        delete _render_display_context._q_image;
        _render_display_context._q_image = nullptr;
    }

    if (_render_display_context._q_image == nullptr)
    {
        #if QT_VERSION >= 0x060000
        _render_display_context._q_image = new QImage((uchar*)_renderer.getImage()->data(), _renderer.getImage()->width(), _renderer.getImage()->height(), QImage::Format_RGBA32FPx4);
        #else
            //Because Qt5 doesn't have the QImage::Format_RGBA32FPx4 format which our image is encoded in
            //we're converting our image to a Format_RGBA8888 which Qt5 supports
            uchar* image_8888_data = new uchar[_renderer.getImage()->width() * _renderer.getImage()->height() * 4];
            for (int i = 0; i < _renderer.getImage()->height(); i++)
            {
                for (int j = 0; j < _renderer.getImage()->width(); j++)
                {
                    uchar r, g, b, a;
                    r = (uchar)(std::clamp(_renderer.getImage()->operator()(j, i).r, 0.0f, 1.0f) * 255);
                    g = (uchar)(std::clamp(_renderer.getImage()->operator()(j, i).g, 0.0f, 1.0f) * 255);
                    b = (uchar)(std::clamp(_renderer.getImage()->operator()(j, i).b, 0.0f, 1.0f) * 255);
                    a = (uchar)(std::clamp(_renderer.getImage()->operator()(j, i).a, 0.0f, 1.0f) * 255);

                    image_8888_data[(i * _renderer.getImage()->width() + j) * 4 + 0] = r;
                    image_8888_data[(i * _renderer.getImage()->width() + j) * 4 + 1] = g;
                    image_8888_data[(i * _renderer.getImage()->width() + j) * 4 + 2] = b;
                    image_8888_data[(i * _renderer.getImage()->width() + j) * 4 + 3] = a;
                }
            }

            this->q_image = new QImage(image_8888_data, _renderer.getImage()->width(), _renderer.getImage()->height(), QImage::Format_RGBA8888);
        #endif
    }

//Flipping the image because our ray tracer produces flipped images
#if QT_VERSION >= 0x060000
    _render_display_context._q_image->mirror();
#else
    QImage mirrored = _render_display_context._q_image->mirrored();
    delete _render_display_context._q_image;
    _render_display_context._q_image = new QImage(mirrored);
#endif
    if (_render_display_context._graphics_scene != nullptr)
        delete _render_display_context._graphics_scene;
    _render_display_context._graphics_scene = new QGraphicsScene(this);

    _render_display_context._graphics_scene->clear();
    _render_display_context._graphics_scene->addPixmap(QPixmap::fromImage(*_render_display_context._q_image));
    QPixmap map = QPixmap::fromImage(*_render_display_context._q_image);

    this->ui->graphics_view->setScene(_render_display_context._graphics_scene);

    timer.stop();
    std::cout << timer.elapsed() << "ms" << std::endl;
    _render_display_context._mutex.unlock();
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
    new_ssao = this->ui->ssao_1_check_box->isChecked();

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

    _renderer.clear_z_buffer();
    _renderer.clear_normal_buffer();
    _renderer.clear_image();
}

void MainWindow::transform_object()
{
    //First checking that there has been some changes in the object transform options
    //before going through all the computations
    if (object_transform_edits_changed())
    {
        Transform object_transform = get_object_transform_from_edits();

        _renderer.set_object_transform(object_transform);
    }
}

void MainWindow::build_camera_to_world_matrix()
{
    Transform ctx_matrix = get_camera_transform_from_edits();

    _renderer.set_camera_transform(ctx_matrix);
}

void MainWindow::on_render_button_clicked()
{
    if (_render_going)
        return;

    std::stringstream ss;

    Timer timer;
    prepare_bvh();//Updates the BVH if necessary
    timer.start();
    prepare_renderer_buffers();//Updates the various buffers of the renderer
    //if necessary
    timer.stop();
    ss << std::endl << "Buffer intialization time: " << timer.elapsed() << "ms" << std::endl;

    timer.start();
    transform_object();
    build_camera_to_world_matrix();
    timer.stop();
    ss << std::endl << "Object transformation time: " << timer.elapsed() << "ms" << std::endl;
    write_to_console(ss);

    _render_thread_handle.start();
}

void MainWindow::precompute_materials(Materials& materials)
{
    for (Material& material : materials.materials)
    {
        float luminance = 0.2126f * material.specular.r + 0.7152f * material.specular.g + 0.0722 * material.specular.b;
        float tau = std::pow(Material::SPECULAR_THRESHOLD_EPSILON / luminance, 1 / material.ns);

        material.specular_threshold = tau;
    }
}

void MainWindow::load_obj(const char* filepath, Transform transform)
{
    std::stringstream ss;
    Timer timer;

    timer.start();

    MeshIOData meshData = read_meshio_data(filepath);
    std::vector<Triangle> triangles = MeshIOUtils::create_triangles(meshData, _renderer.get_materials().count(), transform);

    _renderer.set_triangles(triangles);
    for(const Material& mat : meshData.materials.materials)
        _renderer.get_materials().materials.push_back(mat);
    precompute_materials(_renderer.get_materials());

    //This is used to invalidate the cached object transform so that the
    //transform that is currently written in the UI edits will be reapplied
    //to the new OBJ being loaded. If we were not invalidating the transform
    //the new OBJ wouldn't be coherent with the transform of the edits of the UI
    _cached_object_transform_translation.x = -INFINITY;
    _renderer.reset_previous_transform();

    timer.stop();

    ss << "OBJ Loading time: " << timer.elapsed() << "ms";
    write_to_console(ss);
}

Image MainWindow::load_texture_map(const char* filepath)
{
    return read_image(filepath, true);
}

void MainWindow::write_to_console(const std::stringstream& ss)
{
    std::string ss_string = ss.str();

    this->ui->output_console->append(QString(ss_string.c_str()));
    this->ui->output_console->ensureCursorVisible();
}

bool MainWindow::get_render_going() { return _render_going; }
void MainWindow::set_render_going(bool render_going) { _render_going = render_going; }

Renderer& MainWindow::get_renderer() { return _renderer; }

void MainWindow::on_load_robot_obj_button_clicked()
{
    load_obj("./data/Robot/robot.obj", Identity());

    this->ui->object_translation_edit->setText("0.0/-2.0/-4.0");
    this->ui->object_rotation_edit->setText("0.0/0.0/0.0");
    this->ui->object_scale_edit->setText("1.0/1.0/1.0");
}

void MainWindow::on_load_geometry_obj_button_clicked()
{
    load_obj("./data/geometry.obj", Identity());

    this->ui->object_translation_edit->setText("-1.0/-3.0/-10.0");
    this->ui->object_rotation_edit->setText("0.0/170.0/0.0");
    this->ui->object_scale_edit->setText("2.0/2.0/2.0");
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

    this->_render_display_context._q_image->save(filename);
}

void MainWindow::on_render_width_edit_returnPressed() { on_render_button_clicked(); }
void MainWindow::on_render_height_edit_returnPressed() { on_render_button_clicked(); }

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

void MainWindow::on_ssao_1_sample_count_edit_returnPressed() { on_ssao_1_sample_count_edit_editingFinished(); on_render_button_clicked(); }
void MainWindow::on_ssao_1_radius_edit_returnPressed() { on_ssao_1_radius_edit_editingFinished(); on_render_button_clicked(); }
void MainWindow::on_ssao_1_amount_edit_returnPressed() { on_ssao_1_amount_edit_editingFinished(); on_render_button_clicked(); }

void MainWindow::on_camera_fov_edit_editingFinished()
{
    float fov = safe_text_to_float(this->ui->camera_fov_edit->text());
    if (fov != -1)
        _renderer.change_camera_fov(fov);
}


void MainWindow::on_camera_fov_edit_returnPressed() { on_camera_fov_edit_editingFinished(); on_render_button_clicked(); }

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

void MainWindow::on_shade_obj_material_radio_button_toggled(bool checked)
{
    _renderer.render_settings().shading_method = RenderSettings::ShadingMethod::RT_SHADING;
}

void MainWindow::on_shade_pastels_normals_radio_button_toggled(bool checked)
{
    _renderer.render_settings().shading_method = RenderSettings::ShadingMethod::PASTEL_NORMALS_SHADING;
}

void MainWindow::on_shade_abs_normals_radio_button_toggled(bool checked)
{
    _renderer.render_settings().shading_method = RenderSettings::ShadingMethod::ABS_NORMALS_SHADING;
}

void MainWindow::on_shade_barycentric_radio_button_toggled(bool checked)
{
    _renderer.render_settings().shading_method = RenderSettings::ShadingMethod::BARYCENTRIC_COORDINATES_SHADING;
}

void MainWindow::on_shade_visualize_ao_radio_button_toggled(bool checked)
{
    _renderer.render_settings().shading_method = RenderSettings::ShadingMethod::VISUALIZE_AO;
}

void MainWindow::on_load_ao_map_button_clicked()
{
    QFileDialog dialog(this, "Open AO Map");
    dialog.setFileMode(QFileDialog::ExistingFile);
    dialog.setNameFilter(tr("Image File (*.png *.jpg)"));

    if (dialog.exec())
    {
        QStringList files = dialog.selectedFiles();
        QString file_path = files[0];

        Image ao_map = load_texture_map(file_path.toStdString().c_str());//TODO Image pas en float c'est trop lourd

        _renderer.set_ao_map(ao_map);

        QStringList split_path = file_path.split("/");
        this->ui->ao_map_edit->setText(split_path[split_path.size() - 1]);
    }
}

void MainWindow::on_load_diffuse_map_button_clicked()
{
    QFileDialog dialog(this, "Open Diffuse Map");
    dialog.setFileMode(QFileDialog::ExistingFile);
    dialog.setNameFilter(tr("Image File (*.png *.jpg)"));

    if (dialog.exec())
    {
        QStringList files = dialog.selectedFiles();
        QString file_path = files[0];

        Image diffuse_map = load_texture_map(file_path.toStdString().c_str());//TODO Image pas en float c'est trop lourd

        _renderer.set_diffuse_map(diffuse_map);

        QStringList split_path = file_path.split("/");
        this->ui->diffuse_map_edit->setText(split_path[split_path.size() - 1]);
    }
}

void MainWindow::on_load_obj_file_button_clicked()
{
    QFileDialog dialog(this, "Open OBJ File");
    dialog.setFileMode(QFileDialog::ExistingFile);
    dialog.setNameFilter(tr("OBJ File (*.obj)"));

    if (dialog.exec())
    {
        QStringList files = dialog.selectedFiles();
        QString file_path = files[0];

        load_obj(file_path.toStdString().c_str(), Identity());
    }
}

bool MainWindow::object_transform_edits_changed()
{
    QString translation_text = this->ui->object_translation_edit->text();
    QStringList translation = translation_text.split("/");
    if (translation.size() == 3)
    {
        bool ok_x, ok_y, ok_z;
        float x = safe_text_to_float(translation.at(0), ok_x);
        float y = safe_text_to_float(translation.at(1), ok_y);
        float z = safe_text_to_float(translation.at(2), ok_z);

        if (_cached_object_transform_translation.x != x || _cached_object_transform_translation.y != y || _cached_object_transform_translation.z != z)
            return true;
    }

    QString rotation_text = this->ui->object_rotation_edit->text();
    QStringList rotation = rotation_text.split("/");
    if (rotation.size() == 3)
    {
        bool ok_x, ok_y, ok_z;
        float x = safe_text_to_float(rotation.at(0), ok_x);
        float y = safe_text_to_float(rotation.at(1), ok_y);
        float z = safe_text_to_float(rotation.at(2), ok_z);

        if (_cached_object_transform_rotation.x != x || _cached_object_transform_rotation.y != y || _cached_object_transform_rotation.z != z)
            return true;
    }

    QString scale_text = this->ui->object_scale_edit->text();
    QStringList scale = scale_text.split("/");
    if (scale.size() == 3)
    {
        bool ok_x, ok_y, ok_z;
        float x = safe_text_to_float(scale.at(0), ok_x);
        float y = safe_text_to_float(scale.at(1), ok_y);
        float z = safe_text_to_float(scale.at(2), ok_z);

        if (_cached_object_transform_scale.x != x || _cached_object_transform_scale.y != y || _cached_object_transform_scale.z != z)
            return true;
    }

    return false;
}

Transform MainWindow::get_object_transform_from_edits()
{
    Transform transform = Identity();

    QString translation_text = this->ui->object_translation_edit->text();
    QStringList translation = translation_text.split("/");
    if (translation.size() == 3)
    {
        bool ok_x, ok_y, ok_z;
        float x = safe_text_to_float(translation.at(0), ok_x);
        float y = safe_text_to_float(translation.at(1), ok_y);
        float z = safe_text_to_float(translation.at(2), ok_z);

        if (ok_x && ok_y && ok_z)
            transform = transform(Translation(x, y, z));
        _cached_object_transform_translation = Vector(x, y, z);
    }

    QString rotation_text = this->ui->object_rotation_edit->text();
    QStringList rotation = rotation_text.split("/");
    if (rotation.size() == 3)
    {
        bool ok_x, ok_y, ok_z;
        float x = safe_text_to_float(rotation.at(0), ok_x);
        float y = safe_text_to_float(rotation.at(1), ok_y);
        float z = safe_text_to_float(rotation.at(2), ok_z);

        if (ok_x && ok_y && ok_z)
            transform = transform(RotationZ(z))(RotationY(y))(RotationX(x));
        _cached_object_transform_rotation = Vector(x, y, z);
    }

    QString scale_text = this->ui->object_scale_edit->text();
    QStringList scale = scale_text.split("/");
    if (scale.size() == 3)
    {
        bool ok_x, ok_y, ok_z;
        float x = safe_text_to_float(scale.at(0), ok_x);
        float y = safe_text_to_float(scale.at(1), ok_y);
        float z = safe_text_to_float(scale.at(2), ok_z);

        if (ok_x && ok_y && ok_z)
            transform = transform(Scale(x, y, z));
        _cached_object_transform_scale = Vector(x, y, z);
    }

    return transform;
}

Transform MainWindow::get_camera_transform_from_edits()
{
    Transform transform = Identity();

    QString translation_text = this->ui->camera_translation_edit->text();
    QStringList translation = translation_text.split("/");
    if (translation.size() == 3)
    {
        bool ok_x, ok_y, ok_z;
        float x = safe_text_to_float(translation.at(0), ok_x);
        float y = safe_text_to_float(translation.at(1), ok_y);
        float z = safe_text_to_float(translation.at(2), ok_z);

        if (ok_x && ok_y && ok_z)
            transform = transform(Translation(x, y, z));
    }

    QString rotation_text = this->ui->camera_rotation_edit->text();
    QStringList rotation = rotation_text.split("/");
    if (rotation.size() == 3)
    {
        bool ok_x, ok_y, ok_z;
        float x = safe_text_to_float(rotation.at(0), ok_x);
        float y = safe_text_to_float(rotation.at(1), ok_y);
        float z = safe_text_to_float(rotation.at(2), ok_z);

        if (ok_x && ok_y && ok_z)
            transform = transform(RotationZ(z))(RotationY(y))(RotationX(x));
    }

    return transform;
}

void MainWindow::on_object_translation_edit_returnPressed() { on_render_button_clicked(); }
void MainWindow::on_object_rotation_edit_returnPressed() { on_render_button_clicked(); }
void MainWindow::on_object_scale_edit_returnPressed() { on_render_button_clicked(); }

void MainWindow::on_ao_map_check_box_stateChanged(int checked)
{
    _renderer.render_settings().enable_ao_mapping = checked;

    this->ui->load_ao_map_button->setEnabled(checked);
    this->ui->ao_map_edit->setEnabled(checked);
}

void MainWindow::on_diffuse_map_check_box_stateChanged(int checked)
{
    _renderer.render_settings().enable_diffuse_mapping = checked;

    this->ui->load_diffuse_map_button->setEnabled(checked);
    this->ui->diffuse_map_edit->setEnabled(checked);
}

void MainWindow::on_ssao_1_check_box_stateChanged(int checked)
{
    //We're not changing the render settings here as this will be done
    //lazily upon calling the "on_render_button_clicked" function

    this->ui->ssao_1_sample_count_label->setEnabled(checked);
    this->ui->ssao_1_sample_count_edit->setEnabled(checked);

    this->ui->ssao_1_radius_label->setEnabled(checked);
    this->ui->ssao_1_radius_edit->setEnabled(checked);

    this->ui->ssao_1_amount_label->setEnabled(checked);
    this->ui->ssao_1_amount_edit->setEnabled(checked);
}

void MainWindow::on_camera_rotation_edit_returnPressed() { on_render_button_clicked(); }
void MainWindow::on_camera_translation_edit_returnPressed() { on_render_button_clicked(); }

void MainWindow::on_enable_ambient_checkbox_stateChanged(int checked) { _renderer.render_settings().enable_ambient = checked; }
void MainWindow::on_enable_diffuse_checkbox_stateChanged(int checked) { _renderer.render_settings().enable_diffuse = checked; }
void MainWindow::on_enable_specular_checkbox_stateChanged(int checked) { _renderer.render_settings().enable_specular = checked; }
void MainWindow::on_enable_emissive_checkbox_stateChanged(int checked) { _renderer.render_settings().enable_emissive = checked; }

void MainWindow::on_clear_diffuse_map_button_clicked() { _renderer.clear_diffuse_map(); this->ui->diffuse_map_edit->clear(); }
void MainWindow::on_clear_ao_map_button_clicked(){ _renderer.clear_ao_map(); this->ui->ao_map_edit->clear(); }

void MainWindow::on_clear_scene_button_clicked() { _renderer.clear_geometry(); }

void MainWindow::on_add_sphere_button_clicked()
{
    Point center;
    float radius;

    QString sphere_center_text = this->ui->sphere_center_edit->text();
    QStringList splitted = sphere_center_text.split("/");
    if (splitted.size() == 3)
    {
        bool ok_x, ok_y, ok_z;
        float x = safe_text_to_float(splitted.at(0), ok_x);
        float y = safe_text_to_float(splitted.at(1), ok_y);
        float z = safe_text_to_float(splitted.at(2), ok_z);

        if (ok_x && ok_y && ok_z)
            center = Point(x, y, z);
        else
            return;
    }

    radius = safe_text_to_float(this->ui->sphere_radius_edit->text());
    if (radius != -1)//The input was valid
    {
        _renderer.get_materials().materials.push_back(_added_sphere_material);
        _renderer.add_analytic_shape(Sphere(center, radius, (int)(_renderer.get_materials().materials.size() - 1)));
    }

    _added_sphere_material = Renderer::DEFAULT_MATERIAL;
}

void MainWindow::on_sphere_center_edit_returnPressed() { on_add_sphere_button_clicked(); }
void MainWindow::on_sphere_radius_edit_returnPressed() { on_add_sphere_button_clicked(); }

void MainWindow::on_edit_sphere_material_button_clicked()
{
    EditMaterialDialog material_dialog;
    if (material_dialog.exec())
    {
        _added_sphere_material = material_dialog.get_material();
        return;
    }
}

void MainWindow::on_add_plane_button_clicked()
{
    Point point;
    Vector normal;

    QString plane_point_text = this->ui->plane_point_edit->text();
    QStringList splitted = plane_point_text.split("/");
    if (splitted.size() == 3)
    {
        bool ok_x, ok_y, ok_z;
        float x = safe_text_to_float(splitted.at(0), ok_x);
        float y = safe_text_to_float(splitted.at(1), ok_y);
        float z = safe_text_to_float(splitted.at(2), ok_z);

        if (ok_x && ok_y && ok_z)
            point = Point(x, y, z);
        else
            return;
    }

    QString plane_normal_text = this->ui->plane_normal_edit->text();
    splitted = plane_normal_text.split("/");
    if (splitted.size() == 3)
    {
        bool ok_x, ok_y, ok_z;
        float x = safe_text_to_float(splitted.at(0), ok_x);
        float y = safe_text_to_float(splitted.at(1), ok_y);
        float z = safe_text_to_float(splitted.at(2), ok_z);

        if (ok_x && ok_y && ok_z)
            normal = normalize(Vector(x, y, z));
        else
            return;
    }

    _renderer.get_materials().materials.push_back(_added_plane_material);
    _renderer.add_analytic_shape(Plane(point, normal, (int)(_renderer.get_materials().materials.size() - 1)));

    _added_plane_material = Renderer::DEFAULT_PLANE_MATERIAL;
}


void MainWindow::on_plane_point_edit_returnPressed() { on_add_plane_button_clicked(); }
void MainWindow::on_plane_normal_edit_returnPressed() { on_add_plane_button_clicked(); }

void MainWindow::on_edit_plane_material_button_clicked()
{
    EditMaterialDialog material_dialog;
    if (material_dialog.exec())
    {
        _added_plane_material = material_dialog.get_material();

        return;
    }
}

void MainWindow::on_add_random_sphere_button_clicked()
{
    float center_x = (std::rand() / (float)RAND_MAX * 2 - 1) * 5;//[-5; 5]
    float center_z = std::rand() / (float)RAND_MAX * -11;//[-11; 0]

    float choose_mat = std::rand() / (float)RAND_MAX;

    Point center(center_x + std::rand() / (float)RAND_MAX, -2 + 0.2f, center_z + std::rand() / (float)RAND_MAX);
    if (choose_mat < 0.8f)//Diffuse sphere
    {
        _renderer.get_materials().materials.push_back(Renderer::get_random_diffuse_pastel_material());
        _renderer.add_analytic_shape(Sphere(center, 0.2f, _renderer.get_materials().count() - 1));
    }
//            else if (choose_mat < 0.95f)
//            {
//                metalSpheres.push_back({Sphere{center, 0.2f},
//                                        Metal{0.5f*(1.f+rnd3f()),0.5f*rnd()}});
//            }
//            else //Glass sphere
//                dielectricSpheres.push_back({Sphere{center, 0.2f},
//                                             Dielectric{1.5f}});
}


void MainWindow::on_add_default_plane_button_clicked()
{
    _renderer.get_materials().materials.push_back(Renderer::DEFAULT_PLANE_MATERIAL);
    _renderer.add_analytic_shape(Plane(Point(0, -2, 0), Vector(0, 1, 0), _renderer.get_materials().count() - 1));
}

