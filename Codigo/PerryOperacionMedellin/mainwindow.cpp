#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "jugador.h"

#include <QGraphicsScene>
#include <QGraphicsView>
#include <QTimer>
#include <QKeyEvent>
#include <QEvent>
#include <QBrush>
#include <QColor>
#include <QFrame>
#include "nivel.h"

MainWindow::MainWindow(QWidget *padre)
    : QMainWindow(padre)
    , interfaz(new Ui::MainWindow)
    , escena(new Nivel(this))
    , vista(new QGraphicsView(this))
    , jugador(nullptr)
    , temporizador(new QTimer(this))
{
    interfaz->setupUi(this);

    setWindowTitle("Perry: Operacion Medellin");

    // Configurar el espacio del juego.
    escena->setSceneRect(0, 0, 960, 540);
    escena->setBackgroundBrush(QBrush(QColor(35, 40, 45)));

    // Configurar la vista que muestra la escena.
    vista->setScene(escena);
    vista->setFrameShape(QFrame::NoFrame);
    vista->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    vista->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    vista->setFixedSize(960, 540);

    setCentralWidget(vista);

    interfaz->menubar->hide();
    interfaz->statusbar->hide();

    setFixedSize(960, 540);

    // Crear el jugador una sola vez.
    jugador = new Jugador(escena);
    escena->addItem(jugador);
    jugador->setPos(100, 100);

    // Un temporizador controla las actualizaciones del juego.
    temporizador->setInterval(16);
    temporizador->setTimerType(Qt::PreciseTimer);

    connect(temporizador, &QTimer::timeout,
            this, &MainWindow::actualizarJuego);

    // Recibir los eventos de teclado de la vista.
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

    // Qt libera la escena, la vista y el temporizador.
    // La escena libera al jugador.
}

bool MainWindow::eventFilter(QObject *objeto, QEvent *evento)
{
    if (objeto != vista) {
        return QMainWindow::eventFilter(objeto, evento);
    }

    if (evento->type() == QEvent::FocusOut) {
        // Pausar y limpiar el teclado al cambiar de ventana.
        teclas = 0;
        actualizarDireccion();
        temporizador->stop();
    }
    else if (evento->type() == QEvent::FocusIn) {
        // Reanudar sin acumular el tiempo que estuvo pausado.
        tiempo.start();
        temporizador->start();
    }
    else if (evento->type() == QEvent::KeyPress ||
             evento->type() == QEvent::KeyRelease) {

        QKeyEvent *eventoTeclado = static_cast<QKeyEvent *>(evento);

        quint8 mascara = 0;

        switch (eventoTeclado->key()) {
        case Qt::Key_W:
            mascara = Arriba;
            break;

        case Qt::Key_S:
            mascara = Abajo;
            break;

        case Qt::Key_A:
            mascara = Izquierda;
            break;

        case Qt::Key_D:
            mascara = Derecha;
            break;

        default:
            return QMainWindow::eventFilter(objeto, evento);
        }

        // Ignorar las repeticiones automáticas del teclado.
        if (eventoTeclado->isAutoRepeat()) {
            return true;
        }

        if (evento->type() == QEvent::KeyPress) {
            teclas |= mascara;
        }
        else {
            teclas &= ~mascara;
        }

        actualizarDireccion();
        return true;
    }

    return QMainWindow::eventFilter(objeto, evento);
}

void MainWindow::actualizarDireccion()
{
    const int horizontal =
        ((teclas & Derecha) != 0) - ((teclas & Izquierda) != 0);

    const int vertical =
        ((teclas & Abajo) != 0) - ((teclas & Arriba) != 0);

    jugador->establecerDireccion(QPointF(horizontal, vertical));
}

void MainWindow::actualizarJuego()
{
    const qreal segundosTranscurridos = tiempo.restart() / 1000.0;

    // Limitar el paso ante un retraso prolongado.
    const qreal pasoTiempo =
        qMin(segundosTranscurridos, qreal(0.05));

    jugador->actualizar(pasoTiempo);
}
