#include "mainwindow.h"
#include "vistajuego.h"
#include "popeye.h"
#include "menuanimado.h"
#include "introduccion.h"
#include "ui_mainwindow.h"
#include "configuracion.h"
#include "jugador.h"
#include "nivel.h"
#include "carretera.h"
#include "hacienda.h"
#include "entradahacienda.h"
#include "arenapopeye.h"
#include "desenlace.h"
#include "audiojuego.h"
#include "dificultad.h"
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QSlider>
#include <QListWidget>
#include <QStackedWidget>
#include <QHBoxLayout>

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

MainWindow::MainWindow(QWidget* padre)
    : QMainWindow(padre)
    , interfaz(new Ui::MainWindow)
{
    interfaz->setupUi(this);

    // Objetos Qt, como en el ejemplo de clase.
    audio = new AudioJuego(this);
    audio->establecerPausa(true); // La musica del juego espera al menu.
    escena = new Nivel(this);
    conectarSonido(escena);
    connect(escena, SIGNAL(capturaOAlarmaAgotada()), audio, SLOT(reiniciarPerry()));
    conectarSonido(escena->obtenerJugador());
    connect(escena->obtenerJugador(), SIGNAL(camuflajeCambiado(bool)), audio, SLOT(sonarCamuflaje(bool)));
    vista = new VistaJuego(this);
    jugador = escena->obtenerJugador();
    temporizador = new QTimer(this);
    indicador = new QLabel(this);
    prepararVentana();

    connect(temporizador, SIGNAL(timeout()), this, SLOT(moverFigura()));
    connect(escena, SIGNAL(informacionCambiada(QString)), this, SLOT(mostrarInformacionCasa(QString)));
    connect(escena, SIGNAL(controlesLiberados()), this, SLOT(limpiarTeclado()));
    // La transicion se ejecuta al terminar la actualizacion de la escena.
    connect(escena, SIGNAL(casaSuperada()), this, SLOT(entrarCarretera()), Qt::QueuedConnection);

    temporizador->setTimerType(Qt::PreciseTimer);
    actualizarMarcador();
    tiempo.start();
    temporizador->setInterval(16); // Aproximadamente 60 actualizaciones por segundo.
    mostrarMenu();
    pantallas->setCurrentWidget(introduccion);
    introduccion->iniciar();
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
    controles = new QLabel("WASD: mover    |    C: camuflaje segun modo / cancelar    |    R: reiniciar", this);
    controles->setFixedHeight(28);
    controles->setStyleSheet("background:#17222b; color:#eef3f7; padding-left:8px;");

    QWidget* contenedor = new QWidget(this);
    QVBoxLayout* distribucion = new QVBoxLayout(contenedor);
    distribucion->setContentsMargins(0, 0, 0, 0);
    distribucion->setSpacing(0);
    auto* barra = new QWidget(contenedor);
    barra->setFixedHeight(36);
    auto* opciones = new QHBoxLayout(barra);
    opciones->setContentsMargins(8, 2, 8, 2);
    opciones->addWidget(new QLabel("Casa:", barra));
    selectorDificultad = new QComboBox(barra);
    selectorDificultad->setObjectName("selectorDificultad");
    selectorDificultad->addItems({ "Facil", "Normal", "Dificil" });
    selectorDificultad->setCurrentIndex(1);
    selectorDificultad->setToolTip("Nueva partida abre el menu para confirmar la dificultad.");
    opciones->addWidget(selectorDificultad);
    auto* nuevaPartida = new QPushButton("Nueva partida", barra);
    nuevaPartida->setObjectName("nuevaPartida");
    nuevaPartida->setFocusPolicy(Qt::NoFocus);
    opciones->addWidget(nuevaPartida);
    descripcionDificultad = new QLabel(barra);
    opciones->addWidget(descripcionDificultad, 1);
    sonidoActivo = new QCheckBox("Sonido (M)", barra);
    sonidoActivo->setChecked(true);
    sonidoActivo->setFocusPolicy(Qt::NoFocus);
    opciones->addWidget(sonidoActivo);
    auto* volumen = new QSlider(Qt::Horizontal, barra);
    volumen->setObjectName("volumen");
    volumen->setRange(0, 100);
    volumen->setValue(70);
    volumen->setFixedWidth(85);
    volumen->setFocusPolicy(Qt::NoFocus);
    volumen->setToolTip("Volumen general");
    opciones->addWidget(volumen);
    connect(nuevaPartida, SIGNAL(clicked()), this, SLOT(nuevaPartidaConfigurada()));
    connect(selectorDificultad, SIGNAL(currentIndexChanged(int)), this, SLOT(describirDificultad(int)));
    connect(sonidoActivo, SIGNAL(toggled(bool)), audio, SLOT(establecerHabilitado(bool)));
    connect(volumen, SIGNAL(valueChanged(int)), audio, SLOT(establecerVolumen(int)));
    describirDificultad(1);
    distribucion->addWidget(barra);
    distribucion->addWidget(indicador);
    distribucion->addWidget(vista);
    distribucion->addWidget(controles);
    prepararMenu(contenedor);
    setFixedSize(ConfiguracionCasa::anchoVista, ConfiguracionCasa::altoVista + 120);

    // La vista recibe las teclas; el filtro tambien detecta cuando pierde foco.
    vista->setFocusPolicy(Qt::StrongFocus);
    vista->installEventFilter(this);
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
    if (hacienda)
        hacienda->establecerAgarre(false);
    actualizarDireccion();
}

bool MainWindow::eventFilter(QObject* objeto, QEvent* evento)
{
    if (objeto == listaModos && enMenu
        && (evento->type() == QEvent::KeyPress || evento->type() == QEvent::KeyRelease))
    {
        auto* tecla = static_cast<QKeyEvent*>(evento);
        if (tecla->key() == Qt::Key_Return || tecla->key() == Qt::Key_Enter)
        {
            if (evento->type() == QEvent::KeyPress && !tecla->isAutoRepeat())
                elegirModo();
            return true; // Esta pulsacion solo elige el modo.
        }
        // QListWidget ya permite seleccionar con las flechas arriba y abajo.
    }
    if (objeto != vista)
        return QMainWindow::eventFilter(objeto, evento);

    if (enMenu)
        return QMainWindow::eventFilter(objeto, evento);

    if (evento->type() == QEvent::FocusOut)
    {
        limpiarTeclado();
        temporizador->stop();
        audio->establecerPausa(true);
        setWindowTitle("Perry: Operacion Medellin - Pausa");
    }
    else if (evento->type() == QEvent::FocusIn)
    {
        actualizarMarcador();
        tiempo.start();
        temporizador->start();
        audio->establecerPausa(false);
        actualizarTitulo();
    }
    else if (evento->type() == QEvent::KeyPress || evento->type() == QEvent::KeyRelease)
    {
        QKeyEvent* tecla = static_cast<QKeyEvent*>(evento);
        // La vista tiene el foco; enviamos sus teclas a los metodos de la ventana.
        if (evento->type() == QEvent::KeyPress)
            keyPressEvent(tecla);
        else
            keyReleaseEvent(tecla);
        return tecla->isAccepted();
    }
    return QMainWindow::eventFilter(objeto, evento);
}

void MainWindow::keyPressEvent(QKeyEvent* evento)
{
    if (procesarTecla(evento))
        evento->accept();
    else
        QMainWindow::keyPressEvent(evento);
}

void MainWindow::keyReleaseEvent(QKeyEvent* evento)
{
    if (procesarTecla(evento))
        evento->accept();
    else
        QMainWindow::keyReleaseEvent(evento);
}

bool MainWindow::procesarTecla(QKeyEvent* evento)
{
    if (enMenu) return true;
    // Una pulsacion larga no debe activar varias veces salto o camuflaje.
    if (evento->isAutoRepeat())
        return true;
    const bool presionada = evento->type() == QEvent::KeyPress;
    if (evento->key() == Qt::Key_M)
    {
        if (presionada)
            sonidoActivo->setChecked(!sonidoActivo->isChecked());
        return true;
    }
    if (enDesenlace)
    {
        switch (evento->key())
        {
        case Qt::Key_A:
            teclaIzquierda = presionada;
            actualizarDireccion();
            break;
        case Qt::Key_D:
            teclaDerecha = presionada;
            actualizarDireccion();
            break;
        case Qt::Key_Space:
            if (presionada)
                desenlace->saltar();
            break;
        case Qt::Key_E:
            if (presionada)
                desenlace->activarEscudo();
            break;
        case Qt::Key_Return:
        case Qt::Key_Enter:
            if (presionada)
                desenlace->continuar();
            break;
        case Qt::Key_1:
            if (presionada)
                desenlace->elegir(false);
            break;
        case Qt::Key_2:
            if (presionada)
                desenlace->elegir(true);
            break;
        case Qt::Key_R:
            if (presionada)
                volverCasa();
            break;
        default:
            break;
        }
        return true;
    }
    bool mover = presionada;
    if (!enCarretera && !enHacienda && !enEntrada && !enArena && escena->obtenerEstado() != Nivel::EnJuego)
        mover = false;

    switch (evento->key())
    {
    case Qt::Key_W:
        teclaArriba = mover;
        break;
    case Qt::Key_S:
        teclaAbajo = mover;
        break;
    case Qt::Key_A:
        teclaIzquierda = mover;
        break;
    case Qt::Key_D:
        teclaDerecha = mover;
        break;
    case Qt::Key_R:
        if (presionada)
        {
            if (enArena)
            {
                if (arena->obtenerEstado() == ArenaPopeye::Derrota)
                    reiniciarHacienda();
                else
                {
                    limpiarTeclado();
                    arena->establecerDificultad(escena->dificultadActual());
                    arena->reiniciar();
                    audio->reiniciarMusica(Sonido::Popeye);
                    actualizarMarcador();
                    tiempo.start();
                }
            }
            else if (enEntrada)
            {
                if (entrada->obtenerEstado() == EntradaHacienda::Derrota)
                    reiniciarHacienda();
                else
                {
                    limpiarTeclado();
                    entrada->establecerDificultad(escena->dificultadActual());
                    entrada->reiniciar();
                    audio->reiniciarPerry();
                    actualizarMarcador();
                    tiempo.start();
                }
            }
            else if (enHacienda)
            {
                limpiarTeclado();
                hacienda->establecerDificultad(escena->dificultadActual());
                hacienda->reiniciar();
                audio->reiniciarPerry();
                actualizarMarcador();
                tiempo.start();
            }
            else
                volverCasa();
        }
        return true;
    case Qt::Key_Space:
        if (enArena && presionada)
            arena->saltar();
        else if (enEntrada && presionada)
            entrada->saltar();
        else if (enHacienda && presionada)
            hacienda->saltar();
        return true;
    case Qt::Key_Q:
        if (enArena && presionada)
            arena->activarAgilidad();
        return true;
    case Qt::Key_E:
        if (enArena && presionada)
            arena->activarEscudo();
        if (enHacienda)
            hacienda->establecerAgarre(presionada);
        if (enCarretera && presionada)
            carretera->activarEscudo();
        return true;
    case Qt::Key_Return:
    case Qt::Key_Enter:
        if (enArena && presionada)
            arena->golpear();
        else if (enEntrada && presionada)
            entrada->golpear();
        else if (enCarretera && presionada)
            carretera->activarDimensionador();
        return true;
    case Qt::Key_C:
        if (enEntrada && presionada)
            entrada->transformar();
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
    if (teclaDerecha)
        ++horizontal;
    if (teclaIzquierda)
        --horizontal;
    if (teclaAbajo)
        ++vertical;
    if (teclaArriba)
        --vertical;

    if (enDesenlace)
        desenlace->establecerDireccion(horizontal);
    else if (enArena)
        arena->establecerDireccion(horizontal);
    else if (enEntrada)
        entrada->establecerDireccion(horizontal);
    else if (enHacienda)
        hacienda->establecerDireccion(horizontal);
    else if (enCarretera)
        carretera->establecerDireccion(horizontal);
    else
        jugador->establecerDireccion(QPointF(horizontal, vertical));
}

void MainWindow::moverFigura()
{
    if (enMenu) return;
    // Se mide el tiempo real: la rapidez no depende de cuantos timeout lleguen.
    const qreal segundos = tiempo.restart() / 1000.0;
    if (enDesenlace)
    {
        desenlace->actualizar(segundos);
    }
    else if (enArena)
    {
        arena->actualizar(segundos);
    }
    else if (enEntrada)
    {
        entrada->actualizar(segundos);
    }
    else if (enHacienda)
    {
        hacienda->actualizar(segundos);
        vista->centerOn(hacienda->obtenerJugador());
    }
    else if (enCarretera)
    {
        carretera->actualizar(segundos);
    }
    else
    {
        // Nivel actualiza a Perry, a los guardias y a las llaves.
        escena->actualizar(segundos);
        vista->centerOn(jugador);
    }
    actualizarMarcador();
}

void MainWindow::actualizarMarcador()
{
    if (enDesenlace) vista->mostrarEstado(desenlace->corazonesRestantes());
    else if (enArena) vista->mostrarEstado(arena->corazonesRestantes(), arena->obtenerPopeye()->resistenciaActual());
    else if (enEntrada) vista->mostrarEstado(entrada->corazonesRestantes());
    else if (enHacienda) vista->mostrarEstado(hacienda->vidasRestantes());
    else if (enCarretera) vista->mostrarEstado(carretera->resistenciaActual(), -1, int(carretera->distanciaRecorrida()));
    else vista->mostrarEstado(escena->obtenerEstado() == Nivel::Reiniciando ? 0 : 1);
}

void MainWindow::actualizarTitulo()
{
    if (enMenu)
    {
        setWindowTitle("Perry: Operacion Medellin - Elegir modo");
        return;
    }
    if (enDesenlace)
    {
        setWindowTitle("Perry: Operacion Medellin - Desenlace");
        return;
    }
    if (enArena)
    {
        setWindowTitle("Perry: Operacion Medellin - Combate contra Popeye");
        return;
    }
    if (enEntrada)
    {
        setWindowTitle("Perry: Operacion Medellin - Entrada vigilada");
        return;
    }
    if (enHacienda)
        setWindowTitle("Perry: Operacion Medellin - Hacienda");
    else if (enCarretera)
        setWindowTitle("Perry: Operacion Medellin - Carretera");
    else
        setWindowTitle("Perry: Operacion Medellin - La casa");
}

void MainWindow::mostrarInformacionCasa(const QString& texto)
{
    if (!enCarretera && !enHacienda && !enEntrada && !enArena && !enDesenlace)
        indicador->setText(texto);
}
void MainWindow::mostrarInformacionCarretera(const QString& texto)
{
    if (enCarretera)
        indicador->setText(texto);
}
void MainWindow::mostrarInformacionHacienda(const QString& texto)
{
    if (enHacienda)
        indicador->setText(texto);
}
void MainWindow::mostrarLlegadaHacienda()
{
    if (enCarretera && carretera->obtenerEstado() == Carretera::Llegada)
        controles->setText("Enter: bloquear la entrada y pasar al nivel 2    |    R: volver a la casa");
}
void MainWindow::responderDerrotaCarretera()
{
    if (enCarretera && carretera->obtenerEstado() == Carretera::Derrota)
        volverCasa();
}

void MainWindow::entrarCarretera()
{
    if (enMenu || enCarretera || enHacienda || enEntrada || enArena || enDesenlace
        || escena->obtenerEstado() != Nivel::Completado)
        return;
    limpiarTeclado();
    if (!carretera)
    {
        carretera = new Carretera(this);
        conectarSonido(carretera);
        connect(carretera, SIGNAL(entradaHabilitada()), this, SLOT(entrarHacienda()), Qt::QueuedConnection);
        connect(carretera, SIGNAL(haciendaAlcanzada()), this, SLOT(mostrarLlegadaHacienda()),
            Qt::QueuedConnection);
        connect(carretera, SIGNAL(partidaPerdida()), this, SLOT(responderDerrotaCarretera()),
            Qt::QueuedConnection);
        connect(carretera, SIGNAL(informacionCambiada(QString)), this,
            SLOT(mostrarInformacionCarretera(QString)));
    }
    carretera->establecerDificultad(escena->dificultadActual());
    carretera->reiniciar();
    enCarretera = true;
    audio->establecerMusica(Sonido::Perry);
    vista->setScene(carretera);
    vista->centerOn(carretera->sceneRect().center());
    indicador->setText(carretera->informacionActual());
    controles->setText("A/D: conducir    |    E: escudo    |    Enter: entrar al nivel 2 al llegar a 1200 m    | "
                       "   R: reiniciar");
    setWindowTitle("Perry: Operacion Medellin - Carretera");
    actualizarMarcador();
    tiempo.start();
}
void MainWindow::volverCasa()
{
    if (enMenu) return;
    limpiarTeclado();
    enCarretera = false;
    enHacienda = false;
    enEntrada = false;
    enArena = false;
    enDesenlace = false;
    escena->reiniciarCasa();
    audio->reiniciarPerry();
    vista->setViewportUpdateMode(QGraphicsView::MinimalViewportUpdate);
    vista->setScene(escena);
    vista->centerOn(jugador);
    indicador->setText(escena->informacionActual());
    controles->setText("WASD: mover    |    C: camuflaje segun modo / cancelar    |    R: reiniciar");
    setWindowTitle("Perry: Operacion Medellin - La casa");
    actualizarMarcador();
    tiempo.start();
}

void MainWindow::entrarHacienda()
{
    if (enMenu || !enCarretera || carretera->obtenerEstado() != Carretera::Llegada)
        return;
    limpiarTeclado();
    if (!hacienda)
    {
        hacienda = new Hacienda(this);
        conectarSonido(hacienda);
        connect(hacienda, SIGNAL(vidasAgotadas()), audio, SLOT(reiniciarPerry()));
        conectarSonido(hacienda->obtenerJugador());
        connect(
            hacienda, SIGNAL(informacionCambiada(QString)), this, SLOT(mostrarInformacionHacienda(QString)));
        connect(hacienda, SIGNAL(controlesLiberados()), this, SLOT(limpiarTeclado()));
        connect(hacienda, SIGNAL(lianasSuperadas()), this, SLOT(entrarZonaEscoltas()), Qt::QueuedConnection);
    }
    enCarretera = false;
    enHacienda = true;
    audio->establecerMusica(Sonido::Perry);
    hacienda->establecerDificultad(escena->dificultadActual());
    hacienda->reiniciar();
    // El fondo se desplaza a otra velocidad: repintamos la vista visible.
    vista->setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    vista->setScene(hacienda);
    vista->centerOn(hacienda->obtenerJugador());
    indicador->setText(hacienda->informacionActual());
    controles->setText(
        "A/D: moverse | ESPACIO: saltar/soltar | Mantener E: agarrar liana | R: reiniciar hacienda");
    setWindowTitle("Perry: Operacion Medellin - Hacienda");
    actualizarMarcador();
    tiempo.start();
}

void MainWindow::entrarZonaEscoltas()
{
    if (enMenu || !enHacienda || hacienda->obtenerEstado() != Hacienda::Completado)
        return;
    limpiarTeclado();
    if (!entrada)
    {
        entrada = new EntradaHacienda(this);
        conectarSonido(entrada);
        conectarSonido(entrada->obtenerJugador());
        connect(
            entrada, SIGNAL(informacionCambiada(QString)), this, SLOT(mostrarInformacionEntrada(QString)));
        connect(entrada, SIGNAL(controlesLiberados()), this, SLOT(limpiarTeclado()));
        connect(entrada, SIGNAL(entradaSuperada()), this, SLOT(entrarArena()), Qt::QueuedConnection);
        connect(entrada, SIGNAL(partidaPerdida()), this, SLOT(reiniciarHacienda()), Qt::QueuedConnection);
    }
    enHacienda = false;
    enEntrada = true;
    audio->establecerMusica(Sonido::Perry);
    entrada->establecerDificultad(escena->dificultadActual());
    entrada->reiniciar();
    vista->setViewportUpdateMode(QGraphicsView::MinimalViewportUpdate);
    vista->setScene(entrada);
    vista->centerOn(entrada->sceneRect().center());
    indicador->setText(entrada->informacionActual());
    controles->setText(
        "A/D: mover | ESPACIO: saltar | C: transformarse | Enter: patada / entrar | R: reintentar escoltas");
    setWindowTitle("Perry: Operacion Medellin - Entrada vigilada");
    actualizarMarcador();
    tiempo.start();
}
void MainWindow::reiniciarHacienda()
{
    if (enMenu) return;
    const bool perdioEntrada = enEntrada && entrada->obtenerEstado() == EntradaHacienda::Derrota;
    const bool perdioArena = enArena && arena->obtenerEstado() == ArenaPopeye::Derrota;
    if (!perdioEntrada && !perdioArena)
        return;
    limpiarTeclado();
    enEntrada = false;
    enArena = false;
    enHacienda = true;
    audio->reiniciarPerry();
    hacienda->establecerDificultad(escena->dificultadActual());
    hacienda->reiniciar();
    vista->setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    vista->setScene(hacienda);
    vista->centerOn(hacienda->obtenerJugador());
    indicador->setText(hacienda->informacionActual());
    controles->setText(
        "A/D: moverse | ESPACIO: saltar/soltar | Mantener E: agarrar liana | R: reiniciar hacienda");
    setWindowTitle("Perry: Operacion Medellin - Hacienda");
    actualizarMarcador();
    tiempo.start();
}
void MainWindow::mostrarInformacionEntrada(const QString& texto)
{
    if (enEntrada)
        indicador->setText(texto);
}

void MainWindow::entrarArena()
{
    if (enMenu || !enEntrada || entrada->obtenerEstado() != EntradaHacienda::Completada)
        return;
    limpiarTeclado();
    if (!arena)
    {
        arena = new ArenaPopeye(this);
        conectarSonido(arena);
        connect(arena, SIGNAL(victoriaConseguida()), audio, SLOT(detenerMusica()));
        conectarSonido(arena->obtenerJugador());
        connect(arena, SIGNAL(combateSuperado()), this, SLOT(entrarDesenlace()), Qt::QueuedConnection);
        connect(arena, SIGNAL(informacionCambiada(QString)), this, SLOT(mostrarInformacionArena(QString)));
        connect(arena, SIGNAL(controlesLiberados()), this, SLOT(limpiarTeclado()));
        connect(arena, SIGNAL(partidaPerdida()), this, SLOT(reiniciarHacienda()), Qt::QueuedConnection);
    }
    enEntrada = false;
    enArena = true;
    audio->reiniciarMusica(Sonido::Popeye);
    arena->establecerDificultad(escena->dificultadActual());
    arena->reiniciar();
    vista->setViewportUpdateMode(QGraphicsView::MinimalViewportUpdate);
    vista->setScene(arena);
    vista->setMouseTracking(true);
    vista->viewport()->setMouseTracking(true);
    vista->centerOn(arena->sceneRect().center());
    indicador->setText(arena->informacionActual());
    controles->setText("A/D: mover | ESPACIO: saltar | CLIC: disparar | Enter: golpear | E: escudo | Q: agilidad "
                       "| R: reintentar");
    setWindowTitle("Perry: Operacion Medellin - Combate contra Popeye");
    actualizarMarcador();
    tiempo.start();
}
void MainWindow::mostrarInformacionArena(const QString& texto)
{
    if (enArena)
        indicador->setText(texto);
}

void MainWindow::entrarDesenlace()
{
    if (enMenu || !enArena || arena->obtenerEstado() != ArenaPopeye::Victoria)
        return;
    limpiarTeclado();
    if (!desenlace)
    {
        desenlace = new Desenlace(this);
        conectarSonido(desenlace);
        conectarSonido(desenlace->obtenerJugador());
        connect(desenlace, SIGNAL(informacionCambiada(QString)), this,
            SLOT(mostrarInformacionDesenlace(QString)));
        connect(desenlace, SIGNAL(controlesLiberados()), this, SLOT(limpiarTeclado()));
        connect(desenlace, SIGNAL(juegoPerdido()), this, SLOT(volverCasa()), Qt::QueuedConnection);
        connect(desenlace, SIGNAL(reinicioSolicitado()), this, SLOT(volverCasa()), Qt::QueuedConnection);
    }
    enArena = false;
    enDesenlace = true;
    audio->celebrarVictoriaPopeye();
    desenlace->reiniciar(arena->corazonesRestantes());
    vista->setViewportUpdateMode(QGraphicsView::MinimalViewportUpdate);
    vista->setScene(desenlace);
    vista->centerOn(desenlace->sceneRect().center());
    indicador->setText(desenlace->informacionActual());
    mostrarInformacionDesenlace(desenlace->informacionActual());
    actualizarTitulo();
    actualizarMarcador();
    tiempo.start();
}
void MainWindow::mostrarInformacionDesenlace(const QString& texto)
{
    if (!enDesenlace)
        return;
    indicador->setText(texto);
    const auto estado = desenlace->obtenerEstado();
    // Dialogo y eleccion: solo el audio unico iniciado al entrar, sin fondo.
    if (estado == Desenlace::Combate || estado == Desenlace::Advertencia)
        audio->establecerMusica(Sonido::Accion);
    else if (estado != Desenlace::Dialogo && estado != Desenlace::Eleccion)
        audio->establecerMusica(Sonido::Sigilo);
    if (estado == Desenlace::Eleccion)
        controles->setText("1: rechazar el trato | 2: aceptar el trato | R: reiniciar juego");
    else if (estado == Desenlace::Advertencia || estado == Desenlace::Combate)
        controles->setText("A/D: mover | ESPACIO: saltar | Enter: patada | E: escudo | R: reiniciar juego");
    else if (estado == Desenlace::Victoria)
        controles->setText("Enter: nueva partida");
    else if (estado == Desenlace::Dialogo)
        controles->setText("Enter: continuar | R: reiniciar juego");
    else
        controles->setText("R: reiniciar juego");
}

void MainWindow::conectarSonido(QObject* emisor)
{
    connect(emisor, SIGNAL(sonidoSolicitado(int)), audio, SLOT(reproducir(int)));
}
void MainWindow::nuevaPartidaConfigurada()
{
    mostrarMenu();
}
void MainWindow::describirDificultad(int indice)
{
    if (indice == 0)
        descripcionDificultad->setText("Facil: camuflaje ilimitado, 1 escolta y Popeye con 5 vidas.");
    else if (indice == 2)
        descripcionDificultad->setText("Dificil: sin camuflaje, 2 carros, 4 escoltas y balas que atraviesan barreras.");
    else
        descripcionDificultad->setText("Normal: 2 camuflajes, 2 escoltas y Popeye con 10 vidas.");
}

void MainWindow::prepararMenu(QWidget* pantallaJuego)
{
    pantallas = new QStackedWidget(this);
    pantallas->setObjectName("pantallas");
    auto* menu = new MenuAnimado(pantallas);
    auto* distribucion = new QVBoxLayout(menu);
    distribucion->setContentsMargins(266, 216, 266, 104);
    distribucion->setSpacing(6);
    auto* subtitulo = new QLabel("ELIGE TU DESAFÍO", menu);
    subtitulo->setAlignment(Qt::AlignCenter);
    subtitulo->setStyleSheet("font-family:Arial; font-size:19px; font-weight:bold;"
        " color:#ffe477; background:transparent;");
    distribucion->addWidget(subtitulo);

    listaModos = new QListWidget(menu);
    listaModos->setObjectName("listaModos");
    listaModos->setSelectionMode(QAbstractItemView::SingleSelection);
    listaModos->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    listaModos->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    listaModos->setStyleSheet(
        "QListWidget { background:transparent; border:0; outline:0;"
        " font-family:'Arial Black',Arial; font-size:34px; font-weight:900;"
        " font-style:italic; }"
        "QListWidget::item { background:qlineargradient(x1:0,y1:0,x2:1,y2:1,"
        " stop:0 #174b61,stop:1 #0b243b); color:#e6fcff;"
        " border:2px solid #3b899a; border-radius:18px;"
        " margin:4px 2px; padding:4px 14px; }"
        "QListWidget::item:selected { background:qlineargradient(x1:0,y1:0,x2:1,y2:1,"
        " stop:0 #fff29a,stop:0.48 #ffd447,stop:1 #ffae32);"
        " border:3px solid #fff5ba; color:#102e42; }"
        "QListWidget::item:hover:!selected { border:2px solid #75e9de; }");
    listaModos->addItems({ "FÁCIL", "NORMAL", "DIFÍCIL" });
    for (int i = 0; i < listaModos->count(); ++i) {
        listaModos->item(i)->setSizeHint(QSize(0, 78));
        listaModos->item(i)->setTextAlignment(Qt::AlignCenter);
    }
    listaModos->setFixedHeight(242);
    listaModos->setCurrentRow(1);
    listaModos->installEventFilter(this);
    distribucion->addWidget(listaModos);
    auto* ayuda = new QLabel("↑  ↓  ELEGIR     •     ENTER  JUGAR", menu);
    ayuda->setAlignment(Qt::AlignCenter);
    ayuda->setStyleSheet("font-family:Arial; font-size:14px; font-weight:bold; padding:6px; color:#d4fff1; background:transparent;");
    distribucion->addWidget(ayuda);
    pantallas->addWidget(menu);
    pantallas->addWidget(pantallaJuego);
    introduccion = new Introduccion(pantallas);
    pantallas->addWidget(introduccion);
    connect(introduccion, SIGNAL(terminada()), this, SLOT(mostrarMenu()));
    setCentralWidget(pantallas);
    // La jerarquia de padres Qt libera ambas pantallas y sus controles.
}

void MainWindow::mostrarMenu()
{
    limpiarTeclado();
    enMenu = true;
    temporizador->stop();
    audio->establecerPausa(true);
    audio->detenerMusica();
    pantallas->setCurrentIndex(0);
    listaModos->setCurrentRow(selectorDificultad->currentIndex());
    listaModos->setFocus();
    actualizarTitulo();
}

void MainWindow::elegirModo()
{
    if (!enMenu || (introduccion && introduccion->estaActiva()) || listaModos->currentRow() < 0) return;
    const int elegido = listaModos->currentRow();
    selectorDificultad->setCurrentIndex(elegido);
    escena->establecerDificultad(static_cast<Dificultad>(elegido));
    enMenu = false;
    volverCasa();
    pantallas->setCurrentIndex(1);
    vista->setFocus();
    audio->establecerPausa(false);
    actualizarMarcador();
    tiempo.start();
    temporizador->start();
}
