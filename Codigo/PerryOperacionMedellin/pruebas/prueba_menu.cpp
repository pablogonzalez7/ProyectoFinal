#include "introduccion.h"
#include "mainwindow.h"
#include "nivel.h"
#include "jugador.h"
#include "audiojuego.h"
#include "dificultad.h"
#include <QApplication>
#include <QListWidget>
#include <QStackedWidget>
#include <QPushButton>
#include <QTimer>
#include <QKeyEvent>
#include <QFocusEvent>
#include <QDir>
#include <iostream>
#include <stdexcept>
using namespace std;
void exigir(bool condicion, const char* mensaje)
{
    if (!condicion) throw runtime_error(mensaje);
}
void pulsar(QWidget* destino, int codigo, bool repetida = false)
{
    QKeyEvent presion(QEvent::KeyPress, codigo, Qt::NoModifier, QString(), repetida);
    QApplication::sendEvent(destino, &presion);
    QKeyEvent liberacion(QEvent::KeyRelease, codigo, Qt::NoModifier, QString(), repetida);
    QApplication::sendEvent(destino, &liberacion);
}
int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    try
    {
        MainWindow ventana;
        ventana.show(); ventana.findChild<Introduccion *>()->terminar();
        app.processEvents();
        auto* lista = ventana.findChild<QListWidget*>("listaModos");
        auto* pantallas = ventana.findChild<QStackedWidget*>("pantallas");
        auto* nivel = ventana.findChild<Nivel*>();
        auto* reloj = ventana.findChild<QTimer*>();
        auto* audio = ventana.findChild<AudioJuego*>();
        auto* animacion = ventana.findChild<QTimer*>("animacionMenu");
        auto* vista = ventana.findChild<QGraphicsView*>();
        auto* nueva = ventana.findChild<QPushButton*>("nuevaPartida");
        exigir(lista && pantallas && nivel && reloj && audio && nueva, "Faltan objetos del menu");
        exigir(lista->count() == 3 && lista->currentRow() == 1 && lista->hasFocus(),
            "No muestra tres modos con Normal seleccionado y foco");
        exigir(pantallas->currentIndex() == 0 && !reloj->isActive() && audio->estaPausado(),
            "El juego corre mientras se elige dificultad");
        exigir(animacion && animacion->isActive(), "La animacion del menu no inicia");
        const QPointF posicion = nivel->obtenerJugador()->pos();
        QMetaObject::invokeMethod(&ventana, "moverFigura", Qt::DirectConnection);
        exigir(nivel->obtenerJugador()->pos() == posicion, "El menu mueve al jugador");
        const QString carpeta = argc > 1 ? QString::fromLocal8Bit(argv[1]) : "capturas";
        QDir().mkpath(carpeta);
        exigir(ventana.grab().save(carpeta + "/menu_normal.png"), "No guarda menu");
        pulsar(lista, Qt::Key_Up);
        exigir(lista->currentRow() == 0, "Arriba no elige Facil");
        pulsar(lista, Qt::Key_Up);
        exigir(lista->currentRow() == 0, "Arriba sale de la lista");
        pulsar(lista, Qt::Key_Return, true);
        exigir(pantallas->currentIndex() == 0, "Autorrepeticion inicia partida");
        pulsar(lista, Qt::Key_Return);
        app.processEvents();
        exigir(pantallas->currentIndex() == 1 && nivel->dificultadActual() == Dificultad::Facil,
            "Enter no inicia casa en Facil");
        exigir(reloj->isActive() && !audio->estaPausado() && vista->hasFocus(), "No reanuda juego");
        exigir(!animacion->isActive(), "La animacion sigue consumiendo durante el juego");
        reloj->stop();
        nueva->click();
        app.processEvents();
        exigir(pantallas->currentIndex() == 0 && !reloj->isActive(), "Nueva partida no abre menu");
        exigir(animacion->isActive(), "No reanuda animacion al volver al menu");
        pulsar(lista, Qt::Key_Down);
        pulsar(lista, Qt::Key_Down);
        pulsar(lista, Qt::Key_Down);
        exigir(lista->currentRow() == 2, "Abajo no elige Dificil o sale de lista");
        exigir(nivel->dificultadActual() == Dificultad::Facil, "Aplica dificultad sin confirmar");
        pulsar(lista, Qt::Key_Enter); // Enter numerico.
        app.processEvents();
        exigir(nivel->dificultadActual() == Dificultad::Dificil && vista->scene() == nivel,
            "Enter numerico no inicia Dificil en casa");
        reloj->stop();
        pulsar(vista, Qt::Key_R);
        exigir(nivel->dificultadActual() == Dificultad::Dificil, "Reiniciar pierde modo");
        nueva->click();
        app.processEvents();
        pulsar(lista, Qt::Key_Up);
        pulsar(lista, Qt::Key_Return);
        exigir(nivel->dificultadActual() == Dificultad::Normal, "No permite Normal");
        reloj->stop();
        const int escenas = ventana.findChildren<QGraphicsScene*>().size();
        const int items = nivel->items().size();
        for (int i = 0; i < 20; ++i)
        {
            nueva->click();
            pulsar(lista, Qt::Key_Return);
            reloj->stop();
        }
        exigir(ventana.findChildren<QGraphicsScene*>().size() == escenas
                && nivel->items().size() == items && vista->scene() == nivel
                && ventana.findChild<QListWidget*>("listaModos") == lista,
            "Menu o reinicio recrean escenas o acumulan items");
        exigir(ventana.findChildren<QListWidget*>().size() == 1, "Duplica menu");
        cout << "OK menu inicial, flechas, ambos Enter, tres modos, pausa, reinicio y reutilizacion" << endl;
    }
    catch (const exception& error)
    {
        cerr << "ERROR: " << error.what() << endl;
        return 1;
    }
    return 0;
}
