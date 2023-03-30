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

    void set_render_image(const Image* const image);

    void load_obj(const char* filepath);

private slots:
    void on_renderButton_clicked();

    void on_loadRobotObjButton_clicked();

    void on_hybrid_check_box_stateChanged(int arg1);

    void on_render_wdith_edit_editingFinished();

    void on_render_height_edit_editingFinished();

    void on_clipping_check_box_stateChanged(int arg1);

    void on_enable_ssaa_check_box_stateChanged(int arg1);

    void on_dump_render_to_file_button_clicked();

private:
    Ui::MainWindow *ui;

    Renderer _renderer;
    QImage* q_image = nullptr;//Rendered image displayed on the screen

    Graphics_view_zoom* graphics_view_zoom = nullptr;
};
#endif // MAINWINDOW_H
