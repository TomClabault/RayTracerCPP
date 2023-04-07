#include "QT/mainwindow.h"
#include <QApplication>
#include <QFile>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    //Setting up dark theme
    //QFile styleSheetFile("darkstyle/darkstyle.qss");
    //styleSheetFile.open(QFile::ReadOnly);
    //QString styleSheet = QLatin1String(styleSheetFile.readAll());
    //app.setStyleSheet(styleSheet);

    MainWindow window;
    window.show();
    return app.exec();
}
