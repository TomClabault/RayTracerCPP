#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>

#include "color.h"
#include "materials.h"

namespace Ui {
class EditMaterialDialog;
}

class EditMaterialDialog : public QDialog
{
    Q_OBJECT

public:
    explicit EditMaterialDialog(QWidget *parent = nullptr);
    ~EditMaterialDialog();

    float get_refraction_index();
    float get_shininess();
    float get_reflection_intensity();

    Material get_material();

private:

    QString color_to_string(Color& color);
    void update_color_frames();

    /**
     * @brief get_color Open a Color Chooser Dialog to allow the user to choose a
     * color
     * @param member_color The attribute of this class that will also be
     * updated after the user has chosen a color using the UI dialog
     */
    void get_color(Color& member_color);

private slots:
    void on_diffuse_color_button_clicked();
    void on_specular_color_button_clicked();
    void on_ambient_color_button_clicked();
    void on_emission_color_button_clicked();

private:
    Ui::EditMaterialDialog *ui;

    Color _diffuse_color, _specular_color, _ambient_coeff_color, _emission_color;
};

#endif // DIALOG_H
