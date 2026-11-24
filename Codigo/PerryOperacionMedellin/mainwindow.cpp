#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "configuracion.h"
#include "jugador.h"
#include "nivel.h"
#include "carretera.h"
#include "hacienda.h"

#include <QEvent>
#include <QFont>
#include <QFrame>
#include <QGraphicsView>
#include <QKeyEvent>
#include <QLabel>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

using namespace std;

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
    vista->setFixedSize(ConfiguracionCasa::anchoVista, ConfiguracionCasa::altoVista);
    vista->centerOn(jugador);

    // El indicador queda fuera del mapa y no tapa paredes ni personajes.
    indicador->setFixedHeight(56);
    indicador->setFont(QFont("Arial", 10));
    indicador->setStyleSheet("background:#17222b; color:#eef3f7; padding:6px;");
    indicador->setText(escena->informacionActual());
    controles = new QLabel("WASD: mover    |    C: camuflaje 4 s / cancelar    |    R: reiniciar", this);
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
    setFixedSize(ConfiguracionCasa::anchoVista, ConfiguracionCasa::altoVista + 84);

    connect(escena, &Nivel::informacionCambiada, this, [this](const QString &texto) {
        if (!enCarretera && !enHacienda) indicador->setText(texto);
    });
    // Cambia de escena despues de terminar el ciclo actual de la casa.
    connect(escena, &Nivel::casaSuperada, this, &MainWindow::entrarCarretera, Qt::QueuedConnection);
    connect(escena, &Nivel::controlesLiberados, this, &MainWindow::limpiarTeclado);
    connect(temporizador, &QTimer::timeout, this, &MainWindow::actualizarJuego);
    temporizador->setInterval(16);
    temporizador->setTimerType(Qt::PreciseTimer);

    vista->setFocusPolicy(Qt::StrongFocus);
    vista->installEventFilter(this);
    vista->setFocus();
    vista->centerOn(jugador);
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
    if (hacienda) hacienda->establecerAgarre(false);
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
        setWindowTitle(enHacienda ? "Perry: Operacion Medellin - Hacienda" : enCarretera ? "Perry: Operacion Medellin - Carretera" : "Perry: Operacion Medellin - La casa");
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
                if (enHacienda) { limpiarTeclado(); hacienda->reiniciar(); tiempo.start(); }
                else volverCasa();
            }
            return true;
        case Qt::Key_Space:
            if (enHacienda && presionada && !eventoTeclado->isAutoRepeat()) hacienda->saltar();
            return true;
        case Qt::Key_E:
            if (enHacienda && !eventoTeclado->isAutoRepeat()) hacienda->establecerAgarre(presionada);
            if(enCarretera && presionada && !eventoTeclado->isAutoRepeat()) carretera->activarEscudo();
            return true;
        case Qt::Key_F:
            if(enCarretera && presionada && !eventoTeclado->isAutoRepeat()) carretera->activarDimensionador();
            return true;
        case Qt::Key_C:
            if (presionada && !eventoTeclado->isAutoRepeat()
                && !enCarretera && !enHacienda && escena->obtenerEstado() == Nivel::EnJuego) {
                jugador->alternarCamuflaje();
            }
            return true;
        default:
            return QMainWindow::eventFilter(objeto, evento);
        }

        if (eventoTeclado->isAutoRepeat()) {
            return true;
        }
        if (presionada && (enCarretera || enHacienda || escena->obtenerEstado() == Nivel::EnJuego)) {
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
    if (enHacienda) hacienda->establecerDireccion(horizontal);
    else if (enCarretera) carretera->establecerDireccion(horizontal);
    else jugador->establecerDireccion(QPointF(horizontal, vertical));
}

void MainWindow::actualizarJuego()
{
    const qreal segundosReales = tiempo.restart() / 1000.0;
    if (enHacienda) { hacienda->actualizar(segundosReales); vista->centerOn(hacienda->obtenerJugador()); }
    else if (enCarretera) carretera->actualizar(segundosReales);
    else { escena->actualizar(segundosReales); vista->centerOn(jugador); }
}

void MainWindow::entrarCarretera()
{
    if (enCarretera || enHacienda || escena->obtenerEstado()!=Nivel::Completado) return;
    limpiarTeclado();
    if (!carretera) {
        carretera = new Carretera(this);
        connect(carretera, &Carretera::entradaHabilitada,
                this, &MainWindow::entrarHacienda, Qt::QueuedConnection);
        connect(carretera, &Carretera::haciendaAlcanzada, this, [this] {
            if (enCarretera && carretera->obtenerEstado() == Carretera::Llegada)
                controles->setText("F: bloquear la entrada y pasar al nivel 2    |    R: volver a la casa");
        }, Qt::QueuedConnection);
        connect(carretera, &Carretera::partidaPerdida, this, [this] {
            if(enCarretera && carretera->obtenerEstado()==Carretera::Derrota) volverCasa();
        }, Qt::QueuedConnection);
        connect(carretera, &Carretera::informacionCambiada, this, [this](const QString &texto) {
            if (enCarretera) indicador->setText(texto);
        });
    }
    carretera->reiniciar();
    enCarretera=true;
    vista->setScene(carretera);
    vista->centerOn(carretera->sceneRect().center());
    indicador->setText(carretera->informacionActual());
    controles->setText("A/D: conducir    |    E: escudo    |    F: entrar al nivel 2 al llegar a 1200 m    |    R: reiniciar");
    setWindowTitle("Perry: Operacion Medellin - Carretera");
    tiempo.start();
}
void MainWindow::volverCasa()
{
    limpiarTeclado();
    enCarretera=false;
    enHacienda=false;
    escena->reiniciarCasa();
    vista->setScene(escena);
    vista->centerOn(jugador);
    indicador->setText(escena->informacionActual());
    controles->setText("WASD: mover    |    C: camuflaje 4 s / cancelar    |    R: reiniciar");
    setWindowTitle("Perry: Operacion Medellin - La casa");
    tiempo.start();
}

void MainWindow::entrarHacienda()
{
    if (!enCarretera || carretera->obtenerEstado() != Carretera::Llegada) return;
    limpiarTeclado();
    if (!hacienda) {
        hacienda = new Hacienda(this);
        connect(hacienda, &Hacienda::informacionCambiada, this, [this](const QString &texto) {
            if (enHacienda) indicador->setText(texto);
        });
        connect(hacienda, &Hacienda::controlesLiberados, this, &MainWindow::limpiarTeclado);
    }
    enCarretera = false;
    enHacienda = true;
    hacienda->reiniciar();
    vista->setScene(hacienda);
    vista->centerOn(hacienda->obtenerJugador());
    indicador->setText(hacienda->informacionActual());
    controles->setText("A/D: moverse | ESPACIO: saltar/soltar | Mantener E: agarrar liana | R: reiniciar hacienda");
    setWindowTitle("Perry: Operacion Medellin - Hacienda");
    tiempo.start();
}
