#include "arenapopeye.h"
#include "eventossonido.h"
#include "popeye.h"
#include "jugador.h"
#include "proyectilcombate.h"
#include "recursossprites.h"
#include "configuracionhacienda.h"
#include <QApplication>
#include <QGraphicsEllipseItem>
#include <QGraphicsPixmapItem>
#include <QGraphicsRectItem>
#include <QGraphicsLineItem>
#include <QGraphicsSimpleTextItem>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QtMath>
#include <cmath>
using namespace std;
ArenaPopeye::ArenaPopeye(QObject* padre)
    : QGraphicsScene(padre)
    , jugador(new Jugador(nullptr))
    , popeye(new Popeye(superficies, destinos))
{
    setSceneRect(0, 0, 960, 540);
    fondo = RecursosSprites::cargar(":/imagenes/interior_hacienda.png", QSize(960, 540));
    addItem(jugador);
    jugador->activarModoPlataformas();
    jugador->setZValue(5);
    jugador->setAcceptedMouseButtons(Qt::NoButton);
    addItem(popeye);
    const QPixmap hojaEscobar(":/imagenes/escobar.png");
    auto *escobarSentado=addPixmap(RecursosSprites::cargar(":/imagenes/escobar.png",QSize(64,72),
        QRect(0,0,hojaEscobar.width()/4,hojaEscobar.height())));
    escobarSentado->setPos(892,388);
    escobarSentado->setZValue(3);
    escobarSentado->setAcceptedMouseButtons(Qt::NoButton);
    pisos.reserve(superficies.size());
    for (int i = 0; i < int(superficies.size()); ++i)
    {
        auto* piso = addRect(superficies[i], QPen(QColor("#70594b"), 2), QBrush(QColor("#b39476")));
        piso->setAcceptedMouseButtons(Qt::NoButton);
        piso->setZValue(1);
        pisos.push_back(piso);
    }
    escudo = new QGraphicsEllipseItem(-10, -10, 76, 64, jugador);
    escudo->setPen(QPen(QColor("#60e6ff"), 3));
    escudo->setBrush(QColor(70, 210, 255, 40));
    escudo->setZValue(2);
    zonaGolpe = new QGraphicsRectItem(jugador);
    zonaGolpe->setPen(Qt::NoPen);
    zonaGolpe->setBrush(Qt::NoBrush);
    for (auto& b : balasEnemigas)
    {
        b = new ProyectilCombate;
        addItem(b);
    }
    for (auto& b : balasJugador)
    {
        b = new ProyectilCombate;
        addItem(b);
    }
    miraHorizontal = addLine(QLineF(), QPen(QColor("#18a6b3"), 2));
    miraVertical = addLine(QLineF(), QPen(QColor("#18a6b3"), 2));
    miraHorizontal->setZValue(20);
    miraVertical->setZValue(20);
    aviso = addSimpleText("ATAQUE OSCILATORIO: cambia de plataforma", QFont("Arial", 13, QFont::Bold));
    aviso->setBrush(QColor("#7d174d"));
    aviso->setPos(220, 30);
    aviso->setZValue(20);
    reiniciar();
}
void ArenaPopeye::reiniciar()
{
    estado = EnJuego;
    corazones = 5;
    siguienteIdentificador = 0;
    derrotaEmitida = false;
    continuacionEmitida = false;
    inmunidad = 0;
    esperaDisparo = 0;
    esperaEnemigo = 2;
    tiempoEscudo = 0;
    recargaEscudo = 0;
    tiempoAgilidad = 0;
    tiempoFatiga = 0;
    recargaAgilidad = 0;
    tiempoGolpe = 0;
    recargaGolpe = 0;
    siguienteEspecial = 7;
    avisoEspecial = 0;
    esperaDerrota = 0;
    golpeUsado = false;
    jugador->restablecer();
    jugador->setPos(70, 416);
    jugador->aterrizar(460);
    jugador->setOpacity(1);
    popeye->reiniciar();
    escudo->hide();
    aviso->hide();
    for (auto* b : balasEnemigas)
        b->desactivar();
    for (auto* b : balasJugador)
        b->desactivar();
    zonaGolpe->setBrush(Qt::NoBrush);
    apuntar(QPointF(850, 420));
    emit controlesLiberados();
    publicarInformacion();
}
void ArenaPopeye::establecerDireccion(int horizontal)
{
    jugador->establecerDireccion(QPointF(estado == EnJuego ? qBound(-1, horizontal, 1) : 0, 0));
}
void ArenaPopeye::saltar()
{
    if (estado == EnJuego)
        jugador->saltar();
}
void ArenaPopeye::apuntar(const QPointF& punto)
{
    objetivoMouse = punto;
    miraHorizontal->setLine(punto.x() - 7, punto.y(), punto.x() + 7, punto.y());
    miraVertical->setLine(punto.x(), punto.y() - 7, punto.x(), punto.y() + 7);
}
void ArenaPopeye::mouseMoveEvent(QGraphicsSceneMouseEvent* evento)
{
    apuntar(evento->scenePos());
    evento->accept();
}
void ArenaPopeye::mousePressEvent(QGraphicsSceneMouseEvent* evento)
{
    if (evento->button() == Qt::LeftButton)
    {
        apuntar(evento->scenePos());
        dispararA(objetivoMouse);
        evento->accept();
    }
    else
        QGraphicsScene::mousePressEvent(evento);
}
void ArenaPopeye::dispararA(const QPointF& destino)
{
    if (estado != EnJuego || esperaDisparo > 0)
        return;
    for (auto* bala : balasJugador)
    {
        if (bala->estaActivo())
            continue;
        bala->activar(
            jugador->sceneBoundingRect().center(), destino, false, Directo, ++siguienteIdentificador);
        emit sonidoSolicitado(Sonido::Disparo);
        esperaDisparo = 1;
        return;
    }
}
void ArenaPopeye::activarEscudo()
{
    if (estado != EnJuego || recargaEscudo > 0)
        return;
    emit sonidoSolicitado(Sonido::Escudo);
    tiempoEscudo = 1;
    recargaEscudo = 6;
    escudo->show();
    publicarInformacion();
}
void ArenaPopeye::activarAgilidad()
{
    if (estado != EnJuego || recargaAgilidad > 0)
        return;
    emit sonidoSolicitado(Sonido::Camuflaje);
    tiempoAgilidad = 10;
    tiempoFatiga = 0;
    recargaAgilidad = 40;
    publicarInformacion();
}
void ArenaPopeye::actualizarHabilidades(qreal dt)
{
    tiempoEscudo = qMax(qreal(0), tiempoEscudo - dt);
    recargaEscudo = qMax(qreal(0), recargaEscudo - dt);
    recargaAgilidad = qMax(qreal(0), recargaAgilidad - dt);
    if (tiempoAgilidad > 0)
    {
        tiempoAgilidad = qMax(qreal(0), tiempoAgilidad - dt);
        if (tiempoAgilidad < 1e-8)
        {
            tiempoAgilidad = 0;
            tiempoFatiga = 10;
        }
    }
    else
        tiempoFatiga = qMax(qreal(0), tiempoFatiga - dt);
    jugador->establecerRapidezPlataformas(tiempoAgilidad > 0 ? 350 : tiempoFatiga > 0 ? 180 : 250);
    escudo->setVisible(tiempoEscudo > 0);
}
void ArenaPopeye::golpear()
{
    if (estado==Victoria) {
        if (!continuacionEmitida) {continuacionEmitida=true;emit combateSuperado();}
        return;
    }
    if (estado != EnJuego || recargaGolpe > 0)
        return;
    danoGolpe = jugador->estaEnSuelo() ? 1 : 2;
    if (!jugador->estaEnSuelo())
        jugador->iniciarPatada();
    tiempoGolpe = 0.18;
    recargaGolpe = tiempoAgilidad > 0 ? 0.75 : 1.0;
    golpeUsado = false;
    zonaGolpe->setBrush(QColor(100, 240, 255, 65));
    actualizarGolpe();
    comprobarVictoria();
}
void ArenaPopeye::actualizarGolpe()
{
    if (tiempoGolpe <= 0)
    {
        zonaGolpe->setBrush(Qt::NoBrush);
        return;
    }
    zonaGolpe->setRect(jugador->miraIzquierda() ? QRectF(-22, 14, 36, 28) : QRectF(42, 14, 36, 28));
    if (!golpeUsado && zonaGolpe->collidesWithItem(popeye->obtenerImagen()))
    {
        popeye->recibirDano(danoGolpe);
        emit sonidoSolicitado(Sonido::Impacto);
        golpeUsado = true;
    }
}
void ArenaPopeye::actualizarJugador(qreal dt)
{
    const qreal piesAntes = jugador->sceneBoundingRect().bottom();
    jugador->actualizar(dt);
    jugador->setX(qBound(qreal(0), jugador->x(), qreal(904)));
    const QRectF cuerpo = jugador->sceneBoundingRect();
    if (jugador->estaEnSuelo())
    {
        bool apoyado = false;
        for (const QRectF& piso : superficies)
            if (cuerpo.center().x() >= piso.left() && cuerpo.center().x() <= piso.right()
                && abs(cuerpo.bottom() - piso.top()) < 0.1)
                apoyado = true;
        if (!apoyado)
            jugador->establecerEnSuelo(false);
    }
    if (!jugador->estaEnSuelo() && jugador->velocidadVerticalActual() >= 0)
    {
        qreal altura = 1e9;
        for (const QRectF& piso : superficies)
            if (cuerpo.center().x() >= piso.left() && cuerpo.center().x() <= piso.right()
                && piesAntes <= piso.top() + 0.01 && cuerpo.bottom() >= piso.top())
                altura = qMin(altura, piso.top());
        if (altura < 1e8)
            jugador->aterrizar(altura);
    }
}
void ArenaPopeye::dispararEnemigo(TipoAtaque tipo)
{
    const QPointF origen = popeye->sceneBoundingRect().center();
    for (auto* bala : balasEnemigas)
    {
        if (bala->estaActivo())
            continue;
        bala->activar(origen, jugador->sceneBoundingRect().center(), true, tipo, ++siguienteIdentificador);
        popeye->mostrarDisparo();
        emit sonidoSolicitado(Sonido::Disparo);
        return;
    }
}
void ArenaPopeye::terminarProyectil(ProyectilCombate* bala, ResultadoAtaque resultado)
{
    if (!bala->estaActivo())
        return;
    if (bala->esEnemigo())
    {
        ExperienciaDisparo experiencia = bala->experiencia();
        experiencia.resultado = resultado;
        popeye->aprender(experiencia);
    }
    bala->desactivar();
}
void ArenaPopeye::recibirImpacto()
{
    if (inmunidad > 0 || estado != EnJuego)
        return;
    --corazones;
    emit sonidoSolicitado(Sonido::Impacto);
    inmunidad = 0.9;
    if (!corazones)
    {
        estado = Derrota;
        esperaDerrota = 1.2;
        establecerDireccion(0);
        aviso->hide();
        escudo->hide();
        for (auto* b : balasEnemigas)
            b->desactivar();
        for (auto* b : balasJugador)
            b->desactivar();
        emit controlesLiberados();
    }
}
void ArenaPopeye::actualizarProyectiles(qreal dt)
{
    for (auto* bala : balasJugador)
    {
        if (!bala->estaActivo())
            continue;
        bala->actualizar(dt);
        if (bala->collidesWithItem(popeye->obtenerImagen()))
        {
            popeye->recibirDano(1);
            emit sonidoSolicitado(Sonido::Impacto);
            bala->desactivar();
            continue;
        }
        for (auto* piso : pisos)
            if (bala->collidesWithItem(piso))
            {
                bala->desactivar();
                break;
            }
        if (bala->edadActual() > 5 || !sceneRect().intersects(bala->sceneBoundingRect()))
            bala->desactivar();
    }
    comprobarVictoria();
    if (estado != EnJuego)
        return;
    for (auto* bala : balasEnemigas)
    {
        if (!bala->estaActivo())
            continue;
        bala->actualizar(dt, tiempoAgilidad > 0 ? 0.8 : 1);
        bala->observar(jugador->sceneBoundingRect().center());
        if (tiempoEscudo > 0 && bala->collidesWithItem(escudo))
        {
            terminarProyectil(bala, Bloqueado);
            continue;
        }
        if (bala->collidesWithItem(jugador->obtenerImagen()))
        {
            // Un impacto durante inmunidad tampoco es una evasion del jugador.
            terminarProyectil(bala, inmunidad > 0 ? Bloqueado : Acierto);
            recibirImpacto();
            continue;
        }
        for (auto* piso : pisos)
            if (bala->collidesWithItem(piso))
            {
                terminarProyectil(bala, Fallo);
                break;
            }
        if (bala->edadActual() > 5 || !sceneRect().intersects(bala->sceneBoundingRect()))
            terminarProyectil(bala, Fallo);
    }
}
void ArenaPopeye::comprobarVictoria()
{
    if (estado != EnJuego || popeye->resistenciaActual() > 0)
        return;
    estado = Victoria;
    emit victoriaConseguida();
    establecerDireccion(0);
    aviso->hide();
    escudo->hide();
    zonaGolpe->setBrush(Qt::NoBrush);
    jugador->setOpacity(1);
    for (auto* b : balasEnemigas)
        b->desactivar();
    for (auto* b : balasJugador)
        b->desactivar();
    emit controlesLiberados();
    publicarInformacion();
}
void ArenaPopeye::actualizarPaso(qreal dt)
{
    inmunidad = qMax(qreal(0), inmunidad - dt);
    esperaDisparo = qMax(qreal(0), esperaDisparo - dt);
    recargaGolpe = qMax(qreal(0), recargaGolpe - dt);
    tiempoGolpe = qMax(qreal(0), tiempoGolpe - dt);
    jugador->setOpacity(inmunidad > 0 ? 0.55 : 1);
    actualizarHabilidades(dt);
    actualizarJugador(dt);
    popeye->percibir(jugador->sceneBoundingRect().center());
    popeye->actualizar(dt);
    if (popeye->necesitaMoverse())
        popeye->actuar(popeye->razonar());
    esperaEnemigo -= dt;
    if (esperaEnemigo <= 0)
    {
        dispararEnemigo(popeye->razonar().ataque);
        esperaEnemigo += 1.8;
    }
    if (avisoEspecial > 0)
    {
        avisoEspecial = qMax(qreal(0), avisoEspecial - dt);
        if (avisoEspecial < 1e-8)
        {
            avisoEspecial = 0;
            aviso->hide();
            dispararEnemigo(Oscilatorio);
            siguienteEspecial = 8;
        }
    }
    else
    {
        siguienteEspecial -= dt;
        if (siguienteEspecial <= 0)
        {
            avisoEspecial = 1;
            aviso->show();
            emit sonidoSolicitado(Sonido::Alarma);
        }
    }
    actualizarGolpe();
    actualizarProyectiles(dt);
}
void ArenaPopeye::actualizar(qreal segundos)
{
    if (!isfinite(segundos) || segundos <= 0)
        return;
    if (estado == Victoria)
    {
        // Al terminar, los personajes aterrizan; no quedan suspendidos.
        qreal restante = qMin(segundos, qreal(0.05));
        while (restante > 1e-9)
        {
            const qreal dt = qMin(restante, ConfiguracionHacienda::pasoMaximo);
            actualizarJugador(dt);
            popeye->actualizar(dt);
            restante -= dt;
        }
        return;
    }
    if (estado == Derrota)
    {
        esperaDerrota -= segundos;
        if (esperaDerrota <= 0 && !derrotaEmitida)
        {
            derrotaEmitida = true;
            emit partidaPerdida();
        }
        return;
    }
    qreal restante = qMin(segundos, qreal(0.05));
    while (restante > 1e-9 && estado == EnJuego)
    {
        const qreal dt = qMin(restante, ConfiguracionHacienda::pasoMaximo);
        actualizarPaso(dt);
        restante -= dt;
    }
    publicarInformacion();
}
QString ArenaPopeye::informacionActual() const
{
    if (estado == Victoria)
        return "Popeye incapacitado! Recuperaste el dimensionador.\nEnter: hablar con Escobar | R: repetir combate.";
    if (estado == Derrota)
        return "Sin corazones. Volviendo al inicio de la hacienda...";
    const QString esc = tiempoEscudo > 0 ? "ACTIVO"
        : recargaEscudo > 0              ? QString::number(qCeil(recargaEscudo)) + " s"
                                         : "LISTO (E)";
    const QString agi = tiempoAgilidad > 0 ? QString("ACTIVA %1 s").arg(qCeil(tiempoAgilidad))
        : tiempoFatiga > 0                 ? QString("FATIGA %1 s").arg(qCeil(tiempoFatiga))
        : recargaAgilidad > 0              ? QString::number(qCeil(recargaAgilidad)) + " s"
                                           : "LISTA (Q)";
    return QString("Corazones: %1/5 | Popeye: %2/%5 | Escudo: %3 | Agilidad: %4\nApunta y haz clic (1 "
                   "disparo/s). Enter: puno en suelo, patada en aire. Anticipa sus saltos.")
        .arg(corazones)
        .arg(popeye->resistenciaActual())
        .arg(esc)
        .arg(agi)
        .arg(Popeye::resistenciaMaxima);
}
void ArenaPopeye::publicarInformacion()
{
    const QString texto = informacionActual();
    if (texto != ultimoTexto)
    {
        ultimoTexto = texto;
        emit informacionCambiada(texto);
    }
}
void ArenaPopeye::drawBackground(QPainter* p, const QRectF&)
{
    p->drawPixmap(0, 0, fondo);
}
