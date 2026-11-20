#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QElapsedTimer>

class Nivel;
class QGraphicsView;
class QTimer;
class Jugador;
class QLabel;

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *padre = nullptr);
    ~MainWindow();

protected:
    bool eventFilter(QObject *objeto, QEvent *evento) override;

private slots:
    void actualizarJuego();
    void limpiarTeclado();

private:
    enum TeclaMovimiento : quint8 {
        Arriba    = 1 << 0,
        Abajo     = 1 << 1,
        Izquierda = 1 << 2,
        Derecha   = 1 << 3
    };

    void actualizarDireccion();

    Ui::MainWindow *interfaz;

    Nivel *escena;
    QGraphicsView *vista;
    Jugador *jugador;

    QTimer *temporizador;
    QElapsedTimer tiempo;
    QLabel *indicador;

    quint8 teclas = 0;
};

#endif // MAINWINDOW_H
