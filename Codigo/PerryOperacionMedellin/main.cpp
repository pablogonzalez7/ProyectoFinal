#include "mainwindow.h"

#include <QApplication>
#include <QMessageBox>
#include <exception>

using namespace std;

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    try {
        MainWindow w;
        w.show();
        return a.exec();
    } catch (const exception &error) {
        QMessageBox::critical(nullptr, "No se pudo iniciar el juego",
                              QString::fromUtf8(error.what()));
        return 1;
    }
}
