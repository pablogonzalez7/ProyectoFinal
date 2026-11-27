#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QElapsedTimer>
#include <QTimer>
#include <QGraphicsView>

using namespace std;

class Introduccion;
class Nivel;
class VistaJuego;
class Carretera;
class Hacienda;
class EntradaHacienda;
class ArenaPopeye;
class Desenlace;
class QKeyEvent;
class Jugador;
class QLabel;
class QComboBox;
class QCheckBox;
class AudioJuego;
class QListWidget;
class QStackedWidget;

QT_BEGIN_NAMESPACE
namespace Ui
{
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* padre = nullptr);
    ~MainWindow();

protected:
    void keyPressEvent(QKeyEvent* evento) override;
    void keyReleaseEvent(QKeyEvent* evento) override;
    bool eventFilter(QObject* objeto, QEvent* evento) override;

private slots:
    // QTimer emite timeout() y este slot actualiza los objetos del nivel.
    void moverFigura();
    void nuevaPartidaConfigurada();
    void mostrarMenu();
    void elegirModo();
    void describirDificultad(int indice);
    void mostrarInformacionCasa(const QString& texto);
    void mostrarInformacionCarretera(const QString& texto);
    void mostrarInformacionHacienda(const QString& texto);
    void mostrarLlegadaHacienda();
    void responderDerrotaCarretera();
    void limpiarTeclado();
    void entrarCarretera();
    void volverCasa();
    void entrarHacienda();
    void entrarZonaEscoltas();
    void entrarArena();
    void entrarDesenlace();
    void mostrarInformacionDesenlace(const QString& texto);
    void mostrarInformacionArena(const QString& texto);
    void reiniciarHacienda();
    void mostrarInformacionEntrada(const QString& texto);

private:
    void prepararVentana();
    void prepararMenu(QWidget* pantallaJuego);
    void conectarSonido(QObject* emisor);
    bool procesarTecla(QKeyEvent* evento);
    void actualizarTitulo();
    void actualizarDireccion();
    void actualizarMarcador();

    Ui::MainWindow* interfaz;
    QStackedWidget* pantallas = nullptr;
    QListWidget* listaModos = nullptr;
    Introduccion* introduccion = nullptr;
    bool enMenu = true;

    Nivel* escena;
    VistaJuego* vista;
    Jugador* jugador;

    QTimer* temporizador;
    QElapsedTimer tiempo;
    QLabel* indicador;
    QLabel* controles;
    AudioJuego* audio;
    QComboBox* selectorDificultad;
    QCheckBox* sonidoActivo;
    QLabel* descripcionDificultad;
    Carretera* carretera = nullptr;
    bool enCarretera = false;
    bool enHacienda = false;
    Hacienda* hacienda = nullptr;
    EntradaHacienda* entrada = nullptr;
    bool enEntrada = false;
    ArenaPopeye* arena = nullptr;
    bool enArena = false;
    Desenlace* desenlace = nullptr;
    bool enDesenlace = false;

    // Una variable por tecla: true mientras se mantiene presionada.
    bool teclaArriba = false;
    bool teclaAbajo = false;
    bool teclaIzquierda = false;
    bool teclaDerecha = false;
};

#endif // MAINWINDOW_H
