#include "desenlace.h"
#include "eventossonido.h"
#include "escobar.h"
#include "jugador.h"
#include "proyectil.h"
#include "recursossprites.h"
#include "configuracionhacienda.h"
#include <QGraphicsRectItem>
#include <QGraphicsEllipseItem>
#include <QGraphicsTextItem>
#include <QPainter>
#include <QtMath>
#include <cmath>
using namespace std;

Desenlace::Desenlace(QObject* padre)
    : QGraphicsScene(padre)
    , jugador(new Jugador(nullptr))
    , escobar(new Escobar)
{
    setSceneRect(0, 0, 960, 540);
    fondo = RecursosSprites::cargar(":/imagenes/interior_hacienda.png", QSize(960, 540));
    addRect(0, 460, 960, 80, QPen(QColor("#70594b"), 2), QBrush(QColor("#b39476")));
    addItem(jugador);
    jugador->activarModoPlataformas();
    jugador->setZValue(5);
    addItem(escobar);
    escudo = new QGraphicsEllipseItem(-10, -10, 76, 64, jugador);
    escudo->setPen(QPen(QColor("#60e6ff"), 3));
    escudo->setBrush(QColor(70, 210, 255, 40));
    zonaGolpe = new QGraphicsRectItem(jugador);
    zonaGolpe->setPen(Qt::NoPen);
    zonaGolpe->setBrush(Qt::NoBrush);
    for (auto& bala : balas)
    {
        bala = new Proyectil;
        addItem(bala);
    }
    for (int i = 0; i < 3; ++i)
    {
        anillos[i] = addEllipse(
            0, 0, 96 - i * 18, 132 - i * 18, QPen(QColor("#53e5ef"), 3), QBrush(QColor(54, 80, 180, 75)));
        anillos[i]->setPos(822 + i * 9, 328 + i * 9);
        anillos[i]->setZValue(2);
    }
    panel = addRect(0, 0, 800, 220, QPen(QColor("#4cc9c9"), 2), QBrush(QColor(15, 28, 37, 240)));
    panel->setPos(80, 48);
    panel->setZValue(15);
    textoPanel = new QGraphicsTextItem(panel);
    textoPanel->setPos(22, 18);
    textoPanel->setTextWidth(756);
    textoPanel->setDefaultTextColor(QColor("#f1f6f6"));
    textoPanel->setFont(QFont("Arial", 16));
    reiniciar(6);
}
void Desenlace::reiniciar(int corazonesConservados)
{
    estado = Dialogo;
    corazones = qBound(1, corazonesConservados, 6);
    aceptoTrato = false;
    derrotaEmitida = false;
    reinicioEmitido = false;
    espera = 0;
    esperaDisparo = 0;
    inmunidad = 0;
    tiempoEscudo = 0;
    recargaEscudo = 0;
    tiempoGolpe = 0;
    recargaGolpe = 0;
    tiempoPortal = 0;
    jugador->restablecer();
    jugador->establecerRapidezPlataformas(250);
    jugador->setPos(260, 416);
    jugador->aterrizar(460);
    jugador->setOpacity(1);
    jugador->show();
    escobar->reiniciar();
    escudo->hide();
    limpiarBalas();
    for (auto* anillo : anillos)
        anillo->hide();
    emit controlesLiberados();
    actualizarPanel();
    publicarInformacion();
}
bool Desenlace::permiteMovimiento() const
{
    return estado == Advertencia || estado == Combate;
}
void Desenlace::establecerDireccion(int direccion)
{
    jugador->establecerDireccion(QPointF(permiteMovimiento() ? qBound(-1, direccion, 1) : 0, 0));
}
void Desenlace::saltar()
{
    if (permiteMovimiento())
        jugador->saltar();
}
void Desenlace::continuar()
{
    if (estado == Dialogo)
    {
        estado = Eleccion;
        actualizarPanel();
        publicarInformacion();
    }
    else if (estado == Combate)
        golpear();
    else if (estado == Victoria && !reinicioEmitido)
    {
        reinicioEmitido = true;
        emit reinicioSolicitado();
    }
}
void Desenlace::elegir(bool aceptarTrato)
{
    if (estado != Eleccion)
        return; // Una sola decision por desenlace.
    aceptoTrato = aceptarTrato;
    if (!aceptarTrato)
    {
        empezarEntrega();
        return;
    }
    // Perry se da la vuelta, pero el aviso deja al jugador reaccionar.
    jugador->establecerDireccion(QPointF(-1, 0));
    jugador->establecerDireccion(QPointF());
    escobar->cambiarPose(Escobar::Apuntando);
    estado = Advertencia;
    emit sonidoSolicitado(Sonido::Alarma);
    espera = 1.2;
    emit controlesLiberados();
    actualizarPanel();
    publicarInformacion();
}
void Desenlace::activarEscudo()
{
    if (!permiteMovimiento() || recargaEscudo > 0)
        return;
    tiempoEscudo = 1;
    emit sonidoSolicitado(Sonido::Escudo);
    recargaEscudo = 6;
    escudo->show();
    publicarInformacion();
}
void Desenlace::golpear()
{
    if (estado != Combate || jugador->estaEnSuelo() || recargaGolpe > 0)
        return;
    jugador->iniciarPatada();
    tiempoGolpe = 0.20;
    recargaGolpe = 0.45;
    actualizarGolpe();
}
void Desenlace::actualizarGolpe()
{
    if (estado != Combate || tiempoGolpe <= 0 || jugador->estaEnSuelo())
        return;
    zonaGolpe->setRect(jugador->miraIzquierda() ? QRectF(-22, 26, 36, 18) : QRectF(42, 26, 36, 18));
    if (zonaGolpe->collidesWithItem(escobar->obtenerCabeza()))
    {
        tiempoGolpe = 0;
        escobar->cambiarPose(Escobar::Incapacitado);
        emit sonidoSolicitado(Sonido::Impacto);
        empezarEntrega();
    }
}
void Desenlace::actualizarJugador(qreal dt)
{
    jugador->actualizar(dt);
    jugador->setX(qBound(qreal(0), jugador->x(), qreal(904)));
    if (!jugador->estaEnSuelo() && jugador->velocidadVerticalActual() >= 0
        && jugador->sceneBoundingRect().bottom() >= 460)
        jugador->aterrizar(460);
}
void Desenlace::disparar()
{
    const QPointF origen = escobar->origenDisparo();
    const QPointF destino = jugador->sceneBoundingRect().center();
    for (auto* bala : balas)
    {
        if (bala->estaActivo())
            continue;
        bala->activar(origen, destino, 520);
        emit sonidoSolicitado(Sonido::Disparo);
        const QPointF diferencia = destino - origen;
        bala->setRotation(qRadiansToDegrees(atan2(diferencia.y(), diferencia.x())) + 90);
        return;
    }
}
void Desenlace::recibirImpacto()
{
    if (estado != Combate || inmunidad > 0)
        return;
    --corazones;
    emit sonidoSolicitado(Sonido::Impacto);
    inmunidad = 0.9;
    if (!corazones)
    {
        estado = Derrota;
        emit sonidoSolicitado(Sonido::Derrota);
        espera = 1.5;
        limpiarBalas();
        escudo->hide();
        jugador->establecerDireccion(QPointF());
        emit controlesLiberados();
        actualizarPanel();
    }
    publicarInformacion();
}
void Desenlace::actualizarBalas(qreal dt)
{
    for (auto* bala : balas)
    {
        if (!bala->estaActivo())
            continue;
        bala->actualizar(dt);
        if (tiempoEscudo > 0 && bala->collidesWithItem(escudo))
            bala->desactivar();
        else if (bala->collidesWithItem(jugador->obtenerImagen()))
        {
            bala->desactivar();
            recibirImpacto();
        }
        else if (!sceneRect().intersects(bala->sceneBoundingRect()) || bala->y() > 460)
            bala->desactivar();
    }
}
void Desenlace::limpiarBalas()
{
    for (auto* bala : balas)
        bala->desactivar();
}
void Desenlace::empezarEntrega()
{
    estado = Entrega;
    espera = 1.5;
    tiempoGolpe = 0;
    limpiarBalas();
    escudo->hide();
    jugador->setOpacity(1);
    jugador->establecerDireccion(QPointF());
    if (!aceptoTrato)
        escobar->cambiarPose(Escobar::DePie);
    emit controlesLiberados();
    actualizarPanel();
    publicarInformacion();
}
void Desenlace::empezarPortal()
{
    estado = Portal;
    escobar->hide();
    for (auto* anillo : anillos)
        anillo->show();
    actualizarPanel();
    publicarInformacion();
}
void Desenlace::terminar()
{
    estado = Victoria;
    emit sonidoSolicitado(Sonido::Victoria);
    jugador->hide();
    jugador->establecerDireccion(QPointF());
    actualizarPanel();
    publicarInformacion();
    emit controlesLiberados();
    emit juegoCompletado();
}
void Desenlace::actualizarPaso(qreal dt)
{
    if (estado == Dialogo || estado == Eleccion || estado == Victoria)
        return;
    if (estado == Derrota)
    {
        espera -= dt;
        actualizarJugador(dt);
        if (espera <= 0 && !derrotaEmitida)
        {
            derrotaEmitida = true;
            emit juegoPerdido();
        }
        return;
    }
    if (estado == Entrega)
    {
        actualizarJugador(dt);
        espera -= dt;
        if (espera <= 0)
            empezarPortal();
        return;
    }
    if (estado == Portal)
    {
        tiempoPortal += dt;
        for (int i = 0; i < 3; ++i)
            anillos[i]->setOpacity(0.65 + 0.3 * sin(tiempoPortal * 5 + i));
        const qreal distancia = 870 - jugador->sceneBoundingRect().center().x();
        jugador->establecerDireccion(QPointF(distancia > 0 ? 1 : -1, 0));
        actualizarJugador(dt);
        if (abs(distancia) < 8 && jugador->estaEnSuelo())
            terminar();
        return;
    }
    inmunidad = qMax(qreal(0), inmunidad - dt);
    tiempoEscudo = qMax(qreal(0), tiempoEscudo - dt);
    recargaEscudo = qMax(qreal(0), recargaEscudo - dt);
    tiempoGolpe = qMax(qreal(0), tiempoGolpe - dt);
    recargaGolpe = qMax(qreal(0), recargaGolpe - dt);
    escudo->setVisible(tiempoEscudo > 0);
    jugador->setOpacity(inmunidad > 0 ? 0.55 : 1);
    actualizarJugador(dt);
    escobar->mirarHacia(jugador->sceneBoundingRect().center().x());
    if (estado == Advertencia)
    {
        espera -= dt;
        if (espera <= 0)
        {
            estado = Combate;
            esperaDisparo = 0;
            actualizarPanel();
        }
        return;
    }
    actualizarGolpe();
    if (estado != Combate)
        return;
    esperaDisparo -= dt;
    if (esperaDisparo <= 0)
    {
        disparar();
        esperaDisparo += 0.9;
    }
    actualizarBalas(dt);
}
void Desenlace::actualizar(qreal segundos)
{
    if (!isfinite(segundos) || segundos <= 0)
        return;
    qreal restante = qMin(segundos, qreal(0.05));
    while (restante > 1e-9)
    {
        const qreal dt = qMin(restante, ConfiguracionHacienda::pasoMaximo);
        actualizarPaso(dt);
        restante -= dt;
    }
    publicarInformacion();
}
QString Desenlace::informacionActual() const
{
    if (estado == Dialogo || estado == Eleccion)
        return QString(
            "Dimensionador recuperado | Corazones conservados: %1/6. Tu decision cierra la mision.")
            .arg(corazones);
    if (estado == Advertencia)
        return "TRAICION! Preparate: puedes moverte, saltar y usar el escudo.";
    if (estado == Combate)
    {
        const QString carga = tiempoEscudo > 0 ? "ACTIVO"
            : recargaEscudo > 0                ? QString::number(qCeil(recargaEscudo)) + " s"
                                               : "LISTO (E)";
        return QString("Corazones: %1/6 | Escudo: %2\nSalta y pulsa F para alcanzar su cabeza. Si pierdes, "
                       "vuelves al inicio del juego.")
            .arg(corazones)
            .arg(carga);
    }
    if (estado == Entrega)
        return "Escobar queda bajo custodia. El dimensionador esta listo para volver.";
    if (estado == Portal)
        return "Regresando a casa...";
    if (estado == Victoria)
        return "MISION COMPLETADA | F: nueva partida";
    return "Perry fue capturado. Reiniciando el juego desde la casa...";
}
void Desenlace::actualizarPanel()
{
    QString texto;
    if (estado == Dialogo)
        texto = "DIMENSIONADOR RECUPERADO\n\nPerry: Te llevare a la carcel antes de volver a casa.\n\n[F] "
                "Continuar";
    else if (estado == Eleccion)
        texto = "LA OFERTA DE ESCOBAR\n\nEscobar: Dejame libre. Te dare riqueza, poder y beneficios para tus "
                "duenos.\n\n[1] Rechazar el trato       [2] Aceptar el trato\nAceptar pone en riesgo toda la "
                "mision.";
    else if (estado == Advertencia)
        texto = "TRAICION!\n\nEscobar aprovecha que Perry se da la vuelta y levanta su arma.\n\nA/D: moverse "
                "  ESPACIO: saltar   E: escudo";
    else if (estado == Entrega)
        texto = aceptoTrato
            ? "ESCOBAR INCAPACITADO\n\nPerry supera la traicion y lo entrega a las autoridades."
            : "TRATO RECHAZADO\n\nPerry entrega a Escobar a las autoridades y conserva el dimensionador.";
    else if (estado == Victoria)
        texto = "MISION COMPLETADA\n\nEscobar esta bajo custodia. Perry regreso a su dimension.\n\nGracias "
                "por jugar Perry: Operacion Medellin.\n[F] Nueva partida";
    else if (estado == Derrota)
        texto = "MISION FALLIDA\n\nPerry fue capturado tras la traicion.\nEl juego comenzara de nuevo desde "
                "la casa.";
    panel->setVisible(!texto.isEmpty());
    textoPanel->setPlainText(texto);
}
void Desenlace::publicarInformacion()
{
    const QString texto = informacionActual();
    if (texto != ultimoTexto)
    {
        ultimoTexto = texto;
        emit informacionCambiada(texto);
    }
}
void Desenlace::drawBackground(QPainter* p, const QRectF&)
{
    p->drawPixmap(0, 0, fondo);
}
