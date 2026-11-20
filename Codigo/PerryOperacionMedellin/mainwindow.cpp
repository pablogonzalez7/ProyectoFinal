#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "configuracion.h"
#include "jugador.h"
#include "nivel.h"

#include <QEvent>
#include <QFont>
#include <QFrame>
#include <QGraphicsView>
#include <QKeyEvent>
#include <QLabel>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

MainWindow::MainWindow(QWidget *padre)
    : QMainWindow(padre), interfaz(new Ui::MainWindow)
    , escena(new Nivel(this)), vista(new QGraphicsView(this))
    , jugador(escena->obtenerJugador()), temporizador(new QTimer(this))
    , indicador(new QLabel(this))
{
    interfaz->setupUi(this);
    setWindowTitle("Perry: Operacion Medellin - La casa");
    interfaz->menubar->hide();
    interfaz->statusbar->hide();

    vista->setScene(escena);
    vista->setFrameShape(QFrame::NoFrame);
    vista->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    vista->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    vista->setFixedSize(ConfiguracionCasa::ancho, ConfiguracionCasa::alto);

    // El indicador queda fuera del mapa y no tapa paredes ni personajes.
    indicador->setFixedHeight(56);
    indicador->setFont(QFont("Arial", 10));
    indicador->setStyleSheet("background:#17222b; color:#eef3f7; padding:6px;");
    indicador->setText(escena->informacionActual());
    auto *controles = new QLabel("WASD: mover    |    C: sombrero / camuflaje    |    R: reiniciar", this);
    controles->setFixedHeight(28);
    controles->setStyleSheet("background:#17222b; color:#eef3f7; padding-left:8px;");

    auto *contenedor = new QWidget(this);
    auto *distribucion = new QVBoxLayout(contenedor);
    distribucion->setContentsMargins(0, 0, 0, 0);
    distribucion->setSpacing(0);
    distribucion->addWidget(indicador);
    distribucion->addWidget(vista);
    distribucion->addWidget(controles);
    setCentralWidget(contenedor);
    setFixedSize(ConfiguracionCasa::ancho, ConfiguracionCasa::alto + 84);

    connect(escena, &Nivel::informacionCambiada, indicador, &QLabel::setText);
    connect(escena, &Nivel::controlesLiberados, this, &MainWindow::limpiarTeclado);
    connect(temporizador, &QTimer::timeout, this, &MainWindow::actualizarJuego);
    temporizador->setInterval(16);
    temporizador->setTimerType(Qt::PreciseTimer);

    vista->setFocusPolicy(Qt::StrongFocus);
    vista->installEventFilter(this);
    vista->setFocus();
    tiempo.start();
    temporizador->start();
}

MainWindow::~MainWindow()
{
    temporizador->stop();
    vista->removeEventFilter(this);
    delete interfaz;
    // QObject libera escena/vista/temporizador; la escena libera sus items.
}

void MainWindow::limpiarTeclado()
{
    teclas = 0;
    actualizarDireccion();
}

bool MainWindow::eventFilter(QObject *objeto, QEvent *evento)
{
    if (objeto != vista) {
        return QMainWindow::eventFilter(objeto, evento);
    }
    if (evento->type() == QEvent::FocusOut) {
        limpiarTeclado();
        temporizador->stop();
        setWindowTitle("Perry: Operacion Medellin - Pausa");
    } else if (evento->type() == QEvent::FocusIn) {
        tiempo.start();
        temporizador->start();
        setWindowTitle("Perry: Operacion Medellin - La casa");
    } else if (evento->type() == QEvent::KeyPress || evento->type() == QEvent::KeyRelease) {
        auto *eventoTeclado = static_cast<QKeyEvent *>(evento);
        const bool presionada = evento->type() == QEvent::KeyPress;
        quint8 mascara = 0;

        switch (eventoTeclado->key()) {
        case Qt::Key_W: mascara = Arriba; break;
        case Qt::Key_S: mascara = Abajo; break;
        case Qt::Key_A: mascara = Izquierda; break;
        case Qt::Key_D: mascara = Derecha; break;
        case Qt::Key_R:
            if (presionada && !eventoTeclado->isAutoRepeat()) {
                escena->reiniciarCasa();
                tiempo.start();
            }
            return true;
        case Qt::Key_C:
            if (presionada && !eventoTeclado->isAutoRepeat()
                && escena->obtenerEstado() == Nivel::EnJuego) {
                jugador->alternarCamuflaje();
            }
            return true;
        default:
            return QMainWindow::eventFilter(objeto, evento);
        }

        if (eventoTeclado->isAutoRepeat()) {
            return true;
        }
        if (presionada && escena->obtenerEstado() == Nivel::EnJuego) {
            teclas |= mascara;
        } else {
            teclas &= ~mascara;
        }
        actualizarDireccion();
        return true;
    }
    return QMainWindow::eventFilter(objeto, evento);
}

void MainWindow::actualizarDireccion()
{
    const int horizontal = ((teclas & Derecha) != 0) - ((teclas & Izquierda) != 0);
    const int vertical = ((teclas & Abajo) != 0) - ((teclas & Arriba) != 0);
    jugador->establecerDireccion(QPointF(horizontal, vertical));
}

void MainWindow::actualizarJuego()
{
    const qreal segundosReales = tiempo.restart() / 1000.0;
    escena->actualizar(segundosReales);
}
