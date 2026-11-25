#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "configuracion.h"
#include "jugador.h"
#include "nivel.h"
#include "carretera.h"
#include "hacienda.h"
#include "entradahacienda.h"
#include "arenapopeye.h"

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
{
    interfaz->setupUi(this);

    // Objetos Qt, como en el ejemplo de clase.
    escena = new Nivel(this);
    vista = new QGraphicsView(this);
    jugador = escena->obtenerJugador();
    temporizador = new QTimer(this);
    indicador = new QLabel(this);
    prepararVentana();

    connect(temporizador, SIGNAL(timeout()), this, SLOT(moverFigura()));
    connect(escena, SIGNAL(informacionCambiada(QString)),
            this, SLOT(mostrarInformacionCasa(QString)));
    connect(escena, SIGNAL(controlesLiberados()), this, SLOT(limpiarTeclado()));
    // La transicion se ejecuta al terminar la actualizacion de la escena.
    connect(escena, SIGNAL(casaSuperada()), this, SLOT(entrarCarretera()),
            Qt::QueuedConnection);

    temporizador->setTimerType(Qt::PreciseTimer);
    tiempo.start();
    temporizador->start(16); // Aproximadamente 60 actualizaciones por segundo.
}

void MainWindow::prepararVentana()
{
    setWindowTitle("Perry: Operacion Medellin - La casa");
    interfaz->menubar->hide();
    interfaz->statusbar->hide();

    vista->setViewportUpdateMode(QGraphicsView::MinimalViewportUpdate);
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

    QWidget *contenedor = new QWidget(this);
    QVBoxLayout *distribucion = new QVBoxLayout(contenedor);
    distribucion->setContentsMargins(0, 0, 0, 0);
    distribucion->setSpacing(0);
    distribucion->addWidget(indicador);
    distribucion->addWidget(vista);
    distribucion->addWidget(controles);
    setCentralWidget(contenedor);
    setFixedSize(ConfiguracionCasa::anchoVista, ConfiguracionCasa::altoVista + 84);

    // La vista recibe las teclas; el filtro tambien detecta cuando pierde foco.
    vista->setFocusPolicy(Qt::StrongFocus);
    vista->installEventFilter(this);
    vista->setFocus();
    vista->centerOn(jugador);
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
    teclaArriba = false;
    teclaAbajo = false;
    teclaIzquierda = false;
    teclaDerecha = false;
    if (hacienda) hacienda->establecerAgarre(false);
    actualizarDireccion();
}

bool MainWindow::eventFilter(QObject *objeto, QEvent *evento)
{
    if (objeto != vista) return QMainWindow::eventFilter(objeto,evento);

    if (evento->type() == QEvent::FocusOut) {
        limpiarTeclado();
        temporizador->stop();
        setWindowTitle("Perry: Operacion Medellin - Pausa");
    } else if (evento->type() == QEvent::FocusIn) {
        tiempo.start();
        temporizador->start();
        actualizarTitulo();
    } else if (evento->type() == QEvent::KeyPress || evento->type() == QEvent::KeyRelease) {
        QKeyEvent *tecla = static_cast<QKeyEvent *>(evento);
        // La vista tiene el foco; enviamos sus teclas a los metodos de la ventana.
        if (evento->type() == QEvent::KeyPress) keyPressEvent(tecla);
        else keyReleaseEvent(tecla);
        return tecla->isAccepted();
    }
    return QMainWindow::eventFilter(objeto,evento);
}

void MainWindow::keyPressEvent(QKeyEvent *evento)
{
    if (procesarTecla(evento)) evento->accept();
    else QMainWindow::keyPressEvent(evento);
}

void MainWindow::keyReleaseEvent(QKeyEvent *evento)
{
    if (procesarTecla(evento)) evento->accept();
    else QMainWindow::keyReleaseEvent(evento);
}

bool MainWindow::procesarTecla(QKeyEvent *evento)
{
    // Una pulsacion larga no debe activar varias veces salto o camuflaje.
    if (evento->isAutoRepeat()) return true;
    const bool presionada = evento->type() == QEvent::KeyPress;
    bool mover = presionada;
    if (!enCarretera && !enHacienda && !enEntrada && !enArena && escena->obtenerEstado() != Nivel::EnJuego)
        mover = false;

    switch (evento->key()) {
    case Qt::Key_W: teclaArriba = mover; break;
    case Qt::Key_S: teclaAbajo = mover; break;
    case Qt::Key_A: teclaIzquierda = mover; break;
    case Qt::Key_D: teclaDerecha = mover; break;
    case Qt::Key_R:
        if (presionada) {
            if (enArena) {
                if (arena->obtenerEstado()==ArenaPopeye::Derrota) reiniciarHacienda();
                else {limpiarTeclado();arena->reiniciar();tiempo.start();}
            } else if (enEntrada) {
                if (entrada->obtenerEstado()==EntradaHacienda::Derrota) reiniciarHacienda();
                else {
                    limpiarTeclado();
                    entrada->reiniciar();
                    tiempo.start();
                }
            } else if (enHacienda) {
                limpiarTeclado();
                hacienda->reiniciar();
                tiempo.start();
            } else volverCasa();
        }
        return true;
    case Qt::Key_Space:
        if (enArena && presionada) arena->saltar();
        else if (enEntrada && presionada) entrada->saltar();
        else if (enHacienda && presionada) hacienda->saltar();
        return true;
    case Qt::Key_Q:
        if (enArena && presionada) arena->activarAgilidad();
        return true;
    case Qt::Key_E:
        if (enArena && presionada) arena->activarEscudo();
        if (enHacienda) hacienda->establecerAgarre(presionada);
        if (enCarretera && presionada) carretera->activarEscudo();
        return true;
    case Qt::Key_F:
        if (enArena && presionada) arena->golpear();
        else if (enEntrada && presionada) entrada->golpear();
        else if (enCarretera && presionada) carretera->activarDimensionador();
        return true;
    case Qt::Key_C:
        if (enEntrada && presionada) entrada->transformar();
        else if (presionada && !enArena && !enCarretera && !enHacienda
            && escena->obtenerEstado() == Nivel::EnJuego)
            jugador->alternarCamuflaje();
        return true;
    default:
        return false;
    }
    actualizarDireccion();
    return true;
}

void MainWindow::actualizarDireccion()
{
    int horizontal = 0;
    int vertical = 0;
    if (teclaDerecha) ++horizontal;
    if (teclaIzquierda) --horizontal;
    if (teclaAbajo) ++vertical;
    if (teclaArriba) --vertical;

    if (enArena) arena->establecerDireccion(horizontal);
    else if (enEntrada) entrada->establecerDireccion(horizontal);
    else if (enHacienda) hacienda->establecerDireccion(horizontal);
    else if (enCarretera) carretera->establecerDireccion(horizontal);
    else jugador->establecerDireccion(QPointF(horizontal,vertical));
}

void MainWindow::moverFigura()
{
    // Se mide el tiempo real: la rapidez no depende de cuantos timeout lleguen.
    const qreal segundos = tiempo.restart()/1000.0;
    if (enArena) {
        arena->actualizar(segundos);
    } else if (enEntrada) {
        entrada->actualizar(segundos);
    } else if (enHacienda) {
        hacienda->actualizar(segundos);
        vista->centerOn(hacienda->obtenerJugador());
    } else if (enCarretera) {
        carretera->actualizar(segundos);
    } else {
        // Nivel actualiza a Perry, a los guardias y a las llaves.
        escena->actualizar(segundos);
        vista->centerOn(jugador);
    }
}

void MainWindow::actualizarTitulo()
{
    if (enArena) {setWindowTitle("Perry: Operacion Medellin - Combate contra Popeye");return;}
    if (enEntrada) { setWindowTitle("Perry: Operacion Medellin - Entrada vigilada"); return; }
    if (enHacienda) setWindowTitle("Perry: Operacion Medellin - Hacienda");
    else if (enCarretera) setWindowTitle("Perry: Operacion Medellin - Carretera");
    else setWindowTitle("Perry: Operacion Medellin - La casa");
}

void MainWindow::mostrarInformacionCasa(const QString &texto)
{
    if (!enCarretera && !enHacienda && !enEntrada && !enArena) indicador->setText(texto);
}
void MainWindow::mostrarInformacionCarretera(const QString &texto)
{
    if (enCarretera) indicador->setText(texto);
}
void MainWindow::mostrarInformacionHacienda(const QString &texto)
{
    if (enHacienda) indicador->setText(texto);
}
void MainWindow::mostrarLlegadaHacienda()
{
    if (enCarretera && carretera->obtenerEstado() == Carretera::Llegada)
        controles->setText("F: bloquear la entrada y pasar al nivel 2    |    R: volver a la casa");
}
void MainWindow::responderDerrotaCarretera()
{
    if (enCarretera && carretera->obtenerEstado() == Carretera::Derrota)
        volverCasa();
}

void MainWindow::entrarCarretera()
{
    if (enCarretera || enHacienda || enEntrada || enArena || escena->obtenerEstado()!=Nivel::Completado) return;
    limpiarTeclado();
    if (!carretera) {
        carretera = new Carretera(this);
        connect(carretera, SIGNAL(entradaHabilitada()), this, SLOT(entrarHacienda()),
                Qt::QueuedConnection);
        connect(carretera, SIGNAL(haciendaAlcanzada()), this, SLOT(mostrarLlegadaHacienda()),
                Qt::QueuedConnection);
        connect(carretera, SIGNAL(partidaPerdida()), this, SLOT(responderDerrotaCarretera()),
                Qt::QueuedConnection);
        connect(carretera, SIGNAL(informacionCambiada(QString)),
                this, SLOT(mostrarInformacionCarretera(QString)));
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
    enEntrada=false;
    enArena=false;
    escena->reiniciarCasa();
    vista->setViewportUpdateMode(QGraphicsView::MinimalViewportUpdate);
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
        connect(hacienda, SIGNAL(informacionCambiada(QString)),
                this, SLOT(mostrarInformacionHacienda(QString)));
        connect(hacienda, SIGNAL(controlesLiberados()), this, SLOT(limpiarTeclado()));
        connect(hacienda, SIGNAL(lianasSuperadas()), this, SLOT(entrarZonaEscoltas()),
                Qt::QueuedConnection);
    }
    enCarretera = false;
    enHacienda = true;
    hacienda->reiniciar();
    // El fondo se desplaza a otra velocidad: repintamos la vista visible.
    vista->setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    vista->setScene(hacienda);
    vista->centerOn(hacienda->obtenerJugador());
    indicador->setText(hacienda->informacionActual());
    controles->setText("A/D: moverse | ESPACIO: saltar/soltar | Mantener E: agarrar liana | R: reiniciar hacienda");
    setWindowTitle("Perry: Operacion Medellin - Hacienda");
    tiempo.start();
}

void MainWindow::entrarZonaEscoltas()
{
    if (!enHacienda || hacienda->obtenerEstado()!=Hacienda::Completado) return;
    limpiarTeclado();
    if (!entrada) {
        entrada=new EntradaHacienda(this);
        connect(entrada,SIGNAL(informacionCambiada(QString)),this,SLOT(mostrarInformacionEntrada(QString)));
        connect(entrada,SIGNAL(controlesLiberados()),this,SLOT(limpiarTeclado()));
        connect(entrada,SIGNAL(entradaSuperada()),this,SLOT(entrarArena()),Qt::QueuedConnection);
        connect(entrada,SIGNAL(partidaPerdida()),this,SLOT(reiniciarHacienda()),Qt::QueuedConnection);
    }
    enHacienda=false;
    enEntrada=true;
    entrada->reiniciar();
    vista->setViewportUpdateMode(QGraphicsView::MinimalViewportUpdate);
    vista->setScene(entrada);
    vista->centerOn(entrada->sceneRect().center());
    indicador->setText(entrada->informacionActual());
    controles->setText("A/D: mover | ESPACIO: saltar | C: transformarse | F: patada / entrar | R: reintentar escoltas");
    setWindowTitle("Perry: Operacion Medellin - Entrada vigilada");
    tiempo.start();
}
void MainWindow::reiniciarHacienda()
{
    const bool perdioEntrada=enEntrada && entrada->obtenerEstado()==EntradaHacienda::Derrota;
    const bool perdioArena=enArena && arena->obtenerEstado()==ArenaPopeye::Derrota;
    if (!perdioEntrada && !perdioArena) return;
    limpiarTeclado();
    enEntrada=false;
    enArena=false;
    enHacienda=true;
    hacienda->reiniciar();
    vista->setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    vista->setScene(hacienda);
    vista->centerOn(hacienda->obtenerJugador());
    indicador->setText(hacienda->informacionActual());
    controles->setText("A/D: moverse | ESPACIO: saltar/soltar | Mantener E: agarrar liana | R: reiniciar hacienda");
    setWindowTitle("Perry: Operacion Medellin - Hacienda");
    tiempo.start();
}
void MainWindow::mostrarInformacionEntrada(const QString &texto)
{
    if (enEntrada) indicador->setText(texto);
}

void MainWindow::entrarArena()
{
    if (!enEntrada || entrada->obtenerEstado()!=EntradaHacienda::Completada) return;
    limpiarTeclado();
    if (!arena) {
        arena=new ArenaPopeye(this);
        connect(arena,SIGNAL(informacionCambiada(QString)),this,SLOT(mostrarInformacionArena(QString)));
        connect(arena,SIGNAL(controlesLiberados()),this,SLOT(limpiarTeclado()));
        connect(arena,SIGNAL(partidaPerdida()),this,SLOT(reiniciarHacienda()),Qt::QueuedConnection);
    }
    enEntrada=false;
    enArena=true;
    arena->reiniciar();
    vista->setViewportUpdateMode(QGraphicsView::MinimalViewportUpdate);
    vista->setScene(arena);
    vista->setMouseTracking(true);
    vista->viewport()->setMouseTracking(true);
    vista->centerOn(arena->sceneRect().center());
    indicador->setText(arena->informacionActual());
    controles->setText("A/D: mover | ESPACIO: saltar | CLIC: disparar | F: golpear | E: escudo | Q: agilidad | R: reintentar");
    setWindowTitle("Perry: Operacion Medellin - Combate contra Popeye");
    tiempo.start();
}
void MainWindow::mostrarInformacionArena(const QString &texto)
{
    if (enArena) indicador->setText(texto);
}
