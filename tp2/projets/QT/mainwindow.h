#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "graphicsViewZoom.h"
#include "renderer.h"

#include <sstream>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void prepare_bvh();
    void prepare_renderer_buffers();
    void handle_image_processing();
    void set_render_image(const Image* const image);
    void load_obj(const char* filepath, Transform transform);

    void write_to_console(const std::stringstream& ss);

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

    void on_ssaa_radio_button_toggled(bool checked);

    void on_enable_ssao_1_checkbox_stateChanged(int arg1);
    void on_ssao_1_radius_edit_editingFinished();
    void on_ssao_1_sample_count_edit_editingFinished();
    void on_ssao_1_amount_edit_editingFinished();

    void on_ssao_1_sample_count_edit_returnPressed();
    void on_ssao_1_radius_edit_returnPressed();
    void on_ssao_1_amount_edit_returnPressed();

    void on_camera_fov_edit_returnPressed();

    void on_ssaa_factor_edit_returnPressed();

    void on_bvh_max_leaf_object_edit_returnPressed();

    void on_bvh_max_depth_edit_returnPressed();

    void on_enable_bvh_check_box_stateChanged(int arg1);

    void on_enable_shadows_check_box_stateChanged(int arg1);

    void on_light_position_edit_editingFinished();

    void on_light_position_edit_returnPressed();

    void on_shade_normals_radio_button_toggled(bool checked);

    void on_shade_barycentric_radio_button_toggled(bool checked);

    void on_shade_obj_material_radio_button_toggled(bool checked);

private:
    Ui::MainWindow *ui;

    Renderer _renderer;
    QImage* q_image = nullptr;//QImage displayed on the screen

    bool _rendererd_image_allocated = false;//Whether or not we need to delete the image
    //before allocating a new one to free the old memory
    Image* _renderered_image = nullptr;//Image buffer of the image
    //displayed on the screen

    Graphics_view_zoom* graphics_view_zoom = nullptr;
};
#endif // MAINWINDOW_H
