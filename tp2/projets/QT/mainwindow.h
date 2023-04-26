#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "graphicsViewZoom.h"
#include "renderer.h"

#include <mutex>
#include <sstream>
#include <thread>

#include <QDir>
#include <QThread>

class MainWindow;

/**
 * @brief The DisplayThread is in charge of sending display update events to the MainWindow
 * so that the MainWindow updates the render display
 */
class DisplayThread : public QThread
{
    Q_OBJECT

public:
    DisplayThread(MainWindow* main_window);

    void run() override;

    /**
     * @brief Set a flag in the display thread. If the flag is true, it will prevent the thread from
     * sending further display events to the MainWindow. This is to avoid the MainWindow from being
     * overloaded with updates events and then becoming unresponsive
     * @param ongoing The flag
     */
    void set_update_ongoing(bool ongoing);

signals:
    void update_image();

private:
    MainWindow* _main_window;

    bool _update_ongoing = false;
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

    QImage _mirrored_image_buffer;
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

    void load_ao_map(QString file_path);
    void load_diffuse_map(QString file_path);
    void load_normal_map(QString file_path);
    void load_displacement_map(QString file_path);
    void load_roughness_map(QString file_path);

    void write_to_console(const std::stringstream& ss);

    bool get_render_going();
    void set_render_going(bool render_going);

    Renderer& get_renderer();

private:
    Transform get_object_transform_from_edits();
    Transform get_camera_transform_from_edits();

    void load_skybox_into_renderer(const QString& skybox_folder_path);

private slots:
    void on_render_button_clicked();

    void on_dump_render_to_file_button_clicked();
    void on_hybrid_check_box_stateChanged(int checked);
    void on_clipping_check_box_stateChanged(int checked);

    void on_load_robot_obj_button_clicked();
    void on_load_geometry_obj_button_clicked();

    void on_render_width_edit_returnPressed();
    void on_render_height_edit_returnPressed();

    void on_camera_fov_edit_editingFinished();
    void on_camera_fov_edit_returnPressed();

    void on_ssaa_factor_edit_returnPressed();
    void on_ssaa_radio_button_toggled(bool checked);

    void on_ssao_1_check_box_stateChanged(int checked);

    void on_ssao_1_radius_edit_editingFinished();
    void on_ssao_1_sample_count_edit_editingFinished();
    void on_ssao_1_amount_edit_editingFinished();

    void on_ssao_1_sample_count_edit_returnPressed();
    void on_ssao_1_radius_edit_returnPressed();
    void on_ssao_1_amount_edit_returnPressed();    

    void on_bvh_max_leaf_object_edit_returnPressed();
    void on_bvh_max_depth_edit_returnPressed();
    void on_enable_bvh_check_box_stateChanged(int checked);

    void on_enable_shadows_check_box_stateChanged(int checked);

    void on_light_position_edit_editingFinished();
    void on_light_position_edit_returnPressed();

    void on_shade_obj_material_radio_button_toggled(bool checked);
    void on_shade_abs_normals_radio_button_toggled(bool checked);
    void on_shade_pastels_normals_radio_button_toggled(bool checked);
    void on_shade_barycentric_radio_button_toggled(bool checked);
    void on_shade_visualize_ao_radio_button_toggled(bool checked);

    void on_ao_map_check_box_stateChanged(int checked);
    void on_load_ao_map_button_clicked();

    void on_load_obj_file_button_clicked();

    void on_object_translation_edit_returnPressed();
    void on_object_rotation_edit_returnPressed();
    void on_object_scale_edit_returnPressed();

    void on_camera_rotation_edit_returnPressed();
    void on_camera_translation_edit_returnPressed();

    void on_clear_ao_map_button_clicked();
    void on_load_diffuse_map_button_clicked();
    void on_diffuse_map_check_box_stateChanged(int checked);
    void on_clear_diffuse_map_button_clicked();

    void on_enable_ambient_checkbox_stateChanged(int checked);
    void on_enable_diffuse_checkbox_stateChanged(int checked);
    void on_enable_specular_checkbox_stateChanged(int checked);
    void on_enable_emissive_checkbox_stateChanged(int checked);

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

    void on_load_skybox_button_clicked();

    void on_load_skysphere_button_clicked();

    void on_clear_skybox_button_clicked();

    void on_clear_skysphere_button_clicked();

    void on_skysphere_radio_button_toggled(bool checked);

    void on_skybox_radio_button_toggled(bool checked);

    void on_no_sky_texture_button_toggled(bool checked);

    void on_load_normal_map_button_clicked();

    void on_clear_normal_map_button_clicked();


    void on_load_displacement_map_button_clicked();

    void on_clear_displacement_map_button_clicked();

    void on_displacement_strength_edit_editingFinished();

    void on_displacement_strength_edit_returnPressed();

    void on_parallax_steps_edit_editingFinished();

    void on_parallax_steps_edit_returnPressed();

    void on_roughness_samples_edit_editingFinished();

    void on_roughness_samples_edit_returnPressed();

    void on_maximum_recursion_depth_edit_editingFinished();

    void on_maximum_recursion_depth_edit_returnPressed();

    void on_load_roughness_map_button_clicked();

    void on_clear_roughness_map_button_clicked();

    void on_load_whole_texture_folder_clicked();

    void on_normal_map_check_box_stateChanged(int checked);
    void on_displacement_map_check_box_stateChanged(int checked);
    void on_roughness_map_check_box_stateChanged(int checked);


    void on_go_in_front_button_clicked();

//    void on_go_behind_button_clicked();
//    void on_go_bottom_right_button_clicked();
//    void on_go_bottom_left_button_clicked();
//    void on_go_right_button_clicked();
//    void on_go_left_button_clicked();
//    void on_go_top_left_button_clicked();
//    void on_go_top_right_button_clicked();

    void on_turn_right_around_sphere_button_clicked();
    void on_turn_left_around_sphere_button_clicked();

    void on_progressive_render_refresh_check_box_stateChanged(int arg1);

private:
    Ui::MainWindow *ui;

    //Materials of the sphere and planes that are added
    //These materials are edited by the user using the UI
    Material _added_sphere_material = Renderer::DEFAULT_MATERIAL;
    Material _added_plane_material = Renderer::DEFAULT_PLANE_MATERIAL;

    //See on_turn_around_sphere_button_clicked for a detailed comment on this boolean
    bool _bypass_camera_transform_check;

    Vector _cached_object_transform_translation;
    Vector _cached_object_transform_rotation;
    Vector _cached_object_transform_scale;

    Renderer _renderer;

    //This boolean determines whether or not the signals sent by the display thread
    //will be taken into account and the render display refreshed or not
    bool _enable_progressive_render_refresh = true;
    bool _render_going = false;//Whether or not a render is on-going.
    //Used by the render thread to know if the display should be updated or not

    DisplayThread _display_thread_handle;//Thread that refreshes the display on the Qt interface
    RenderThread _render_thread_handle;//Thread that runs the ray tracing renderer
    RenderDisplayContext _render_display_context;
};
#endif // MAINWINDOW_H
