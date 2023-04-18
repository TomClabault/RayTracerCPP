#include "EditMaterialDialog.h"
#include "ui_editMaterialDialog.h"

#include "materials.h".h"

#include <QColorDialog>

EditMaterialDialog::EditMaterialDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::EditMaterialDialog)
{
    ui->setupUi(this);

    _diffuse_color = Color(0.9f);
    _specular_color = Color(1.0f);
    _ambient_coeff_color = Color(1.0f);
    _emission_color = Color(0.0f);

    connect(this->ui->diffuse_color_frame, &ClickableColorQFrame::clicked, this, &EditMaterialDialog::on_diffuse_color_button_clicked);
    connect(this->ui->specular_color_frame, &ClickableColorQFrame::clicked, this, &EditMaterialDialog::on_specular_color_button_clicked);
    connect(this->ui->ambient_color_frame, &ClickableColorQFrame::clicked, this, &EditMaterialDialog::on_ambient_color_button_clicked);
    connect(this->ui->emission_color_frame, &ClickableColorQFrame::clicked, this, &EditMaterialDialog::on_emission_color_button_clicked);

    update_color_frames();
}

EditMaterialDialog::~EditMaterialDialog()
{
    delete ui;
}

float EditMaterialDialog::get_refraction_index()
{
    bool ok;

    float refraction_index = this->ui->refraction_index_edit->text().toFloat(&ok);
    if (!ok)
        return -1;
    else
        return refraction_index;
}

float EditMaterialDialog::get_shininess()
{
    bool ok;

    float shininess = this->ui->shininess_edit->text().toFloat(&ok);
    if (!ok)
        return -1;
    else
        return shininess;
}

float EditMaterialDialog::get_reflection_intensity()
{
    bool ok;

    float reflection_intensity = this->ui->reflection_intensity_edit->text().toFloat(&ok);
    if (!ok)
        return -1;
    else
        return reflection_intensity;
}

Material EditMaterialDialog::get_material()
{
    Material mat;

    mat.ambient_coeff = _ambient_coeff_color;
    mat.diffuse = _diffuse_color;
    mat.diffuse_texture = -1;
    mat.emission = _emission_color;
    mat.ni = get_refraction_index();
    if (mat.ni == -1)
        mat.ni = 1.0;//Default value
    mat.ns = get_shininess();
    if (mat.ns == -1)
        mat.ns = 10;//Default value
    mat.reflection = get_reflection_intensity();
    if (mat.reflection == -1)
        mat.reflection = 0.0f;//Default value
    mat.ns_texture = -1;
    mat.specular_texture = -1;
    mat.specular = _specular_color;

    return mat;
}

QString EditMaterialDialog::color_to_string(Color& color)
{
    std::string std_string = "rgb(" + std::to_string((int)(color.r * 255))
                             + ", " + std::to_string((int)(color.g * 255))
                             + ", " + std::to_string((int)(color.b * 255)) + ")";

    return QString(std_string.c_str());
}

void EditMaterialDialog::update_color_frames()
{
    QString diffuse_color_string = color_to_string(_diffuse_color);
    QString specular_color_string = color_to_string(_specular_color);
    QString ambient_color_string = color_to_string(_ambient_coeff_color);
    QString emission_color_string = color_to_string(_emission_color);

    this->ui->diffuse_color_frame->setStyleSheet("background-color: " + diffuse_color_string + "; border: 1px solid black;");
    this->ui->specular_color_frame->setStyleSheet("background-color: " + specular_color_string + "; border: 1px solid black;");
    this->ui->ambient_color_frame->setStyleSheet("background-color: " + ambient_color_string + "; border: 1px solid black;");
    this->ui->emission_color_frame->setStyleSheet("background-color: " + emission_color_string + "; border: 1px solid black;");
}

void EditMaterialDialog::get_color(Color& member_color)
{
    QColorDialog color_dialog;
    if(color_dialog.exec())
    {
        QColor q_chosen_color = color_dialog.currentColor();
        Color chosen_color = Color(q_chosen_color.redF(), q_chosen_color.greenF(), q_chosen_color.blueF());
        std::cout << chosen_color << std::endl;

        member_color = chosen_color;

        update_color_frames();
    }
}

void EditMaterialDialog::on_diffuse_color_button_clicked() { get_color(_diffuse_color); }
void EditMaterialDialog::on_specular_color_button_clicked() { get_color(_specular_color); }
void EditMaterialDialog::on_ambient_color_button_clicked() { get_color(_ambient_coeff_color); }
void EditMaterialDialog::on_emission_color_button_clicked() { get_color(_emission_color); }
