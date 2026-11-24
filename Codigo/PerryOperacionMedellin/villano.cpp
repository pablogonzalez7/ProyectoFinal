#include "villano.h"
#include "recursossprites.h"
#include "configuracion.h"
#include "jugador.h"
#include "nivel.h"

#include <QBrush>
#include <QColor>
#include <QGraphicsPathItem>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QtMath>
#include <stdexcept>
#include <cmath>

using namespace std;

namespace {
qreal longitudCuadrada(const QPointF &p)
{
    return p.x() * p.x() + p.y() * p.y();
}
}

Villano::Villano(const Nivel *nivelActual, const Jugador *objetivo,
                 const QList<QPointF> &puntosPatrulla)
    : Personaje(ConfiguracionCasa::rapidezPatrulla)
    , nivel(nivelActual), jugador(objetivo), patrulla(puntosPatrulla)
    , cono(new QGraphicsPathItem(this))
{
    if (!nivel || !jugador || patrulla.size() < 2) {
        throw invalid_argument("Un guardia necesita nivel, jugador y al menos dos puntos de patrulla.");
    }
    const QPixmap hoja(":/imagenes/policias.png");
    if (hoja.isNull()) throw runtime_error("No se pudo cargar la hoja de policias.");
    for (int columna = 0; columna < 4; ++columna) {
        for (int fila = 0; fila < 2; ++fila) {
            const int ancho = hoja.width() / 4, alto = hoja.height() / 2;
            // Cada celda contiene la misma figura; se excluyen los margenes.
            const QRect recorte(columna * ancho + ancho * 22 / 100,
                                fila * alto + alto * 4 / 100,
                                ancho * 56 / 100, alto * 92 / 100);
            sprites[columna][fila] = RecursosSprites::cargar(":/imagenes/policias.png", QSize(26,40), recorte);
        }
    }
    camino.reserve(ConfiguracionCasa::totalCeldas + 2);
    setAcceptedMouseButtons(Qt::NoButton);
    cono->setAcceptedMouseButtons(Qt::NoButton);
    cono->setPen(QPen(Qt::NoPen));
    cono->setZValue(-1);
    for (int i = 0; i < cantidadRayos; ++i) {
        const qreal angulo = qDegreesToRadians(-ConfiguracionCasa::semianguloVision
            + 2.0 * ConfiguracionCasa::semianguloVision * i / (cantidadRayos - 1));
        direccionesRayos[i] = QPointF(qCos(angulo), qSin(angulo));
    }
    connect(jugador, &Jugador::camuflajeCambiado, this, &Villano::responderCamuflaje);
    restablecer();
}

QPointF Villano::centro() const { return pos() + boundingRect().center(); }
Villano::Estado Villano::obtenerEstado() const { return estado; }

qreal Villano::radioDeteccion() const
{
    // La linterna conserva su alcance; el disfraz impide reconocer a Perry.
    return ConfiguracionCasa::radioVision;
}

void Villano::responderCamuflaje(bool activo)
{
    esperaPercepcion = 0.0;
    if (activo && estado != Patrullando) {
        cambiarEstado(Patrullando);
        esperaBusqueda = 0.0;
        esperaPatrulla = 0.0;
        ultimaPosicionConocida = centro();
        irHacia(patrulla.at(indicePatrulla));
        actualizarCono(true);
    }
}

void Villano::cambiarEstado(Estado nuevoEstado)
{
    if (estado != nuevoEstado) {
        estado = nuevoEstado;
        update();
        emit estadoCambiado();
    }
}

void Villano::restablecer()
{
    alarma = false;
    pasoAnimacion = 0;
    cambiarEstado(Patrullando);
    velocidad = ConfiguracionCasa::rapidezPatrulla;
    setPos(patrulla.first() - boundingRect().center());
    indicePatrulla = 1;
    const QPointF hacia = patrulla.at(1) - patrulla.first();
    const qreal longitud = qSqrt(longitudCuadrada(hacia));
    direccionVista = longitud > 0.0 ? hacia / longitud : QPointF(1, 0);
    ultimaPosicionConocida = centro();
    esperaPercepcion = 0.0;
    esperaBusqueda = 0.0;
    esperaPatrulla = 0.0;
    irHacia(patrulla.at(indicePatrulla));
    actualizarCono(true);
}

void Villano::establecerAlarma(bool activa)
{
    alarma = activa;
    actualizarCono(true);
}

bool Villano::puedeVerJugador() const
{
    if (jugador->estaCamuflado()) {
        return false;
    }
    const QPointF origen = centro();
    const QPointF objetivo = jugador->sceneBoundingRect().center();
    const QPointF diferencia = objetivo - origen;
    const qreal distancia2 = longitudCuadrada(diferencia);
    const qreal radio = radioDeteccion();
    if (distancia2 > radio * radio) {
        return false;
    }
    if (distancia2 > 1.0e-8) {
        const qreal producto = QPointF::dotProduct(direccionVista, diferencia);
        const qreal coseno = qCos(qDegreesToRadians(ConfiguracionCasa::semianguloVision));
        if (producto <= 0.0 || producto * producto < distancia2 * coseno * coseno) {
            return false;
        }
    }
    return nivel->hayLineaDeVision(origen, objetivo);
}

void Villano::irHacia(const QPointF &destino)
{
    nivel->calcularRuta(centro(), destino, camino);
    pasoRuta = 0;
}

void Villano::comprobarJugador()
{
    if (puedeVerJugador()) {
        const QPointF observado = jugador->sceneBoundingRect().center();
        const bool nuevaPersecucion = estado != Persiguiendo;
        const bool cambioDestino = longitudCuadrada(observado - ultimaPosicionConocida) >= 16.0;
        ultimaPosicionConocida = observado;
        cambiarEstado(Persiguiendo);
        esperaPatrulla = 0.0;
        esperaBusqueda = 0.0;
        if (nuevaPersecucion || cambioDestino || pasoRuta >= camino.size()) {
            irHacia(ultimaPosicionConocida);
        }
    } else if (estado == Persiguiendo) {
        cambiarEstado(Investigando);
        esperaBusqueda = 0.0;
        // Solo se investiga la ultima posicion vista, no la posicion oculta actual.
        irHacia(ultimaPosicionConocida);
    }
}

void Villano::seguirRuta(qreal segundosTranscurridos)
{
    qreal presupuesto = velocidad * segundosTranscurridos;
    while (presupuesto > 1.0e-8 && pasoRuta < camino.size()) {
        const QPointF diferencia = camino.at(pasoRuta) - centro();
        const qreal distancia = qSqrt(longitudCuadrada(diferencia));
        if (distancia < 0.01) {
            ++pasoRuta;
            continue;
        }
        const qreal longitud = qMin(presupuesto, distancia);
        const QPointF deseado = diferencia * (longitud / distancia);
        const QPointF permitido = nivel->limitarDesplazamiento(sceneBoundingRect(), deseado);
        const qreal avance2 = longitudCuadrada(permitido);
        if (avance2 < 1.0e-12) {
            return;
        }
        pasoAnimacion = fmod(pasoAnimacion + qSqrt(avance2) / 12.0, 2.0);
        setPos(pos() + permitido);
        direccionVista = permitido / qSqrt(avance2);
        presupuesto -= longitud;
        if (longitudCuadrada(camino.at(pasoRuta) - centro()) < 0.0001) {
            ++pasoRuta;
        }
    }
}

void Villano::actualizar(qreal segundosTranscurridos)
{
    if (segundosTranscurridos <= 0.0) {
        return;
    }
    esperaPercepcion -= segundosTranscurridos;
    if (esperaPercepcion <= 0.0) {
        esperaPercepcion = ConfiguracionCasa::periodoPercepcion;
        comprobarJugador();
    }
    velocidad = estado == Patrullando ? ConfiguracionCasa::rapidezPatrulla
                                     : ConfiguracionCasa::rapidezPersecucion;
    if (alarma) {
        velocidad *= ConfiguracionCasa::factorAlarma;
    }
    // La percepcion sigue activa durante las pausas; no son periodos de ceguera.
    if (estado == Patrullando && esperaPatrulla > 0.0) {
        pasoAnimacion = 0;
        update();
        esperaPatrulla -= segundosTranscurridos;
        actualizarCono();
        return;
    }
    seguirRuta(segundosTranscurridos);
    if (pasoRuta >= camino.size()) {
        if (estado == Patrullando) {
            esperaPatrulla = ConfiguracionCasa::pausaPatrulla;
            indicePatrulla = (indicePatrulla + 1) % patrulla.size();
            irHacia(patrulla.at(indicePatrulla));
        } else if (estado == Investigando) {
            esperaBusqueda += segundosTranscurridos;
            if (esperaBusqueda >= ConfiguracionCasa::duracionBusqueda) {
                cambiarEstado(Patrullando);
                irHacia(patrulla.at(indicePatrulla));
            }
        }
    }
    actualizarCono();
}

void Villano::actualizarCono(bool forzar)
{
    const qreal radio = radioDeteccion();
    if (!forzar && longitudCuadrada(pos() - ultimaPosicionCono) < 16.0
        && longitudCuadrada(direccionVista - ultimaDireccionCono) < 0.0001
        && radio == ultimoRadioCono && estado == ultimoEstadoCono
        && alarma == ultimaAlarmaCono) {
        return;
    }
    QPainterPath forma;
    const QPointF origen = centro();
    forma.moveTo(boundingRect().center());
    for (const QPointF &rayo : direccionesRayos) {
        const QPointF girado(direccionVista.x() * rayo.x() - direccionVista.y() * rayo.y(),
                             direccionVista.y() * rayo.x() + direccionVista.x() * rayo.y());
        forma.lineTo(nivel->recortarRayo(origen, origen + girado * radio) - pos());
    }
    forma.closeSubpath();
    cono->setPath(forma);
    const QColor color = estado == Persiguiendo ? QColor(255, 65, 65, 75)
        : (alarma || estado == Investigando ? QColor(255, 155, 35, 65)
                                            : QColor(255, 225, 95, 50));
    cono->setBrush(QBrush(color));
    ultimaPosicionCono = pos();
    ultimaDireccionCono = direccionVista;
    ultimoRadioCono = radio;
    ultimoEstadoCono = estado;
    ultimaAlarmaCono = alarma;
}

void Villano::paint(QPainter *pintor, const QStyleOptionGraphicsItem *, QWidget *)
{
    int orientacion = direccionVista.y() < 0 ? 1 : 0;
    if (qAbs(direccionVista.x()) >= qAbs(direccionVista.y()))
        orientacion = direccionVista.x() < 0 ? 2 : 3;
    pintor->drawPixmap(3, 0, sprites[orientacion][int(pasoAnimacion) % 2]);
    if (estado == Persiguiendo) {
        pintor->setPen(QColor("#ff786b"));
        pintor->drawText(QRectF(24,0,8,14), Qt::AlignCenter, "!");
    }
}
