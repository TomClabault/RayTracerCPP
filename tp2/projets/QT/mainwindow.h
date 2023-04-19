#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "graphicsViewZoom.h"
#include "renderer.h"

#include <mutex>
#include <sstream>
#include <thread>

#include <QThread>

class MainWindow;

class DisplayThread : public QThread
{
    Q_OBJECT

public:
    DisplayThread(MainWindow* main_window);

    void run() override;

signals:
    void update_image();

private:
    MainWindow* _main_window;
};

class RenderThread : public QThread
{
    Q_OBJECT

public:
    RenderThread(MainWindow* main_window);

    void run() override;

signals:
    void update_image();

private:
    MainWindow* _main_window;

    std::stringstream _ss;
};

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

struct RenderDisplayContext
{
    QGraphicsScene* _graphics_scene = nullptr;
    Graphics_view_zoom* _graphics_view_zoom = nullptr;

    QImage* _q_image = nullptr;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void setup_render_display_context();

    void update_render_image();

    void prepare_bvh();
    void prepare_renderer_buffers();
    void transform_object();
    void build_camera_to_world_matrix();
    bool object_transform_edits_changed();

    /**
     * @brief Does some pre-computations on the materials. Notably the specular visiblity threshold
     * @param materials The materials
     */
    void precompute_materials(Materials& materials);

    void load_obj(const char* filepath, Transform transform);
    Image load_texture_map(const char* filepath);

    void write_to_console(const std::stringstream& ss);

    bool get_render_going();
    void set_render_going(bool render_going);

    Renderer& get_renderer();

private:
    Transform get_object_transform_from_edits();
    Transform get_camera_transform_from_edits();

private slots:
    void on_render_button_clicked();

    void on_dump_render_to_file_button_clicked();
    void on_hybrid_check_box_stateChanged(int arg1);
    void on_clipping_check_box_stateChanged(int arg1);

    void on_load_robot_obj_button_clicked();
    void on_load_geometry_obj_button_clicked();

    void on_render_width_edit_returnPressed();
    void on_render_height_edit_returnPressed();

    void on_camera_fov_edit_editingFinished();
    void on_camera_fov_edit_returnPressed();

    void on_ssaa_factor_edit_returnPressed();
    void on_ssaa_radio_button_toggled(bool checked);

    void on_ssao_1_check_box_stateChanged(int arg1);

    void on_ssao_1_radius_edit_editingFinished();
    void on_ssao_1_sample_count_edit_editingFinished();
    void on_ssao_1_amount_edit_editingFinished();

    void on_ssao_1_sample_count_edit_returnPressed();
    void on_ssao_1_radius_edit_returnPressed();
    void on_ssao_1_amount_edit_returnPressed();    

    void on_bvh_max_leaf_object_edit_returnPressed();
    void on_bvh_max_depth_edit_returnPressed();
    void on_enable_bvh_check_box_stateChanged(int arg1);

    void on_enable_shadows_check_box_stateChanged(int arg1);

    void on_light_position_edit_editingFinished();
    void on_light_position_edit_returnPressed();

    void on_shade_obj_material_radio_button_toggled(bool checked);
    void on_shade_abs_normals_radio_button_toggled(bool checked);
    void on_shade_pastels_normals_radio_button_toggled(bool checked);
    void on_shade_barycentric_radio_button_toggled(bool checked);
    void on_shade_visualize_ao_radio_button_toggled(bool checked);

    void on_ao_map_check_box_stateChanged(int arg1);
    void on_load_ao_map_button_clicked();

    void on_load_obj_file_button_clicked();

    void on_object_translation_edit_returnPressed();
    void on_object_rotation_edit_returnPressed();
    void on_object_scale_edit_returnPressed();

    void on_camera_rotation_edit_returnPressed();
    void on_camera_translation_edit_returnPressed();

    void on_clear_ao_map_button_clicked();
    void on_load_diffuse_map_button_clicked();
    void on_diffuse_map_check_box_stateChanged(int arg1);
    void on_clear_diffuse_map_button_clicked();

    void on_enable_ambient_checkbox_stateChanged(int arg1);
    void on_enable_diffuse_checkbox_stateChanged(int arg1);
    void on_enable_specular_checkbox_stateChanged(int arg1);
    void on_enable_emissive_checkbox_stateChanged(int arg1);

    void on_clear_scene_button_clicked();

    void on_add_sphere_button_clicked();
    void on_sphere_center_edit_returnPressed();
    void on_sphere_radius_edit_returnPressed();
    void on_edit_sphere_material_button_clicked();

    void on_add_plane_button_clicked();

    void on_plane_point_edit_returnPressed();

    void on_plane_normal_edit_returnPressed();

    void on_edit_plane_material_button_clicked();

    void on_add_random_sphere_button_clicked();

    void on_add_default_plane_button_clicked();

private:
    Ui::MainWindow *ui;

    //Materials of the sphere and planes that are added
    //These materials are edited by the user using the UI
    Material _added_sphere_material = Renderer::DEFAULT_MATERIAL;
    Material _added_plane_material = Renderer::DEFAULT_PLANE_MATERIAL;

    Vector _cached_object_transform_translation;
    Vector _cached_object_transform_rotation;
    Vector _cached_object_transform_scale;

    Renderer _renderer;

    bool _render_going = false;//Whether or not a render is on-going.
    //Used by the render thread to know if the display should be updated or not

    DisplayThread _display_thread_handle;//Thread that refreshes the display on the Qt interface
    RenderThread _render_thread_handle;//Thread that runs the ray tracing renderer
    RenderDisplayContext _render_display_context;
};
#endif // MAINWINDOW_H
