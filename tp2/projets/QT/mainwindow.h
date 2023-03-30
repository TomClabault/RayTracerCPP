#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "graphicsViewZoom.h"
#include "renderer.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void prepare_renderer_buffers();

    void set_render_image(const Image* const image);

    void load_obj(const char* filepath);

private slots:
    void on_renderButton_clicked();

    void on_hybrid_check_box_stateChanged(int arg1);

    void on_clipping_check_box_stateChanged(int arg1);

    void on_enable_ssaa_check_box_stateChanged(int arg1);

    void on_dump_render_to_file_button_clicked();

    void on_camera_fov_spin_box_valueChanged(int arg1);

    void on_load_robot_obj_button_clicked();

    void on_render_width_edit_returnPressed();

    void on_render_height_edit_returnPressed();

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
