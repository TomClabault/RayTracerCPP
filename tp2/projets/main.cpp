//TODO mettre les fonctions de mainUtils pour qu'elles renvoient des long lng int plutpot que des floats: les milliseoncdes sont pas en float

#include "QT/mainwindow.h"
#include <QApplication>
#include <QFile>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    //Setting up dark theme
//    QFile styleSheetFile("stylesheets/dark_style.qss");
//    styleSheetFile.open(QFile::ReadOnly);
//    QString styleSheet = QLatin1String(styleSheetFile.readAll());
//    app.setStyleSheet(styleSheet);

    MainWindow window;
    window.show();
    return app.exec();
}
