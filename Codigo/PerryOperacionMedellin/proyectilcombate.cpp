#include "proyectilcombate.h"
#include "recursossprites.h"
#include <QPainter>
#include <QtMath>
#include <cmath>
using namespace std;
ProyectilCombate::ProyectilCombate()
    : sprite(RecursosSprites::cargar(":/imagenes/bala.png", QSize(8, 14)))
{
    setZValue(8);
    setAcceptedMouseButtons(Qt::NoButton);
    hide();
}
QRectF ProyectilCombate::boundingRect() const
{
    return QRectF(-8, -8, 16, 16);
}
void ProyectilCombate::activar(
    const QPointF& inicio, const QPointF& destino, bool hostil, TipoAtaque tipo, int identificador)
{
    origen = inicio;
    enemigo = hostil;
    tiempo = 0;
    edad = 0;
    const QPointF delta = destino - inicio;
    const qreal longitud = qSqrt(QPointF::dotProduct(delta, delta));
    velocidad = longitud > 0.001 ? delta * ((hostil ? 280.0 : 520.0) / longitud) : QPointF(520, 0);
    if (tipo == Oscilatorio)
        velocidad = QPointF(delta.x() < 0 ? -180 : 180, 0);
    registro = ExperienciaDisparo();
    registro.identificador = identificador;
    registro.tipo = tipo;
    registro.distancia = longitud < 230 ? 0 : longitud < 480 ? 1 : 2;
    registro.posicionInicial = destino;
    setPos(origen);
    activo = true;
    show();
}
void ProyectilCombate::actualizar(qreal segundos, qreal factorVelocidad)
{
    if (!activo)
        return;
    edad += segundos;
    tiempo += segundos * factorVelocidad;
    QPointF posicion = origen + velocidad * tiempo;
    // La trayectoria se fija al disparar. Nunca vuelve a leer al objetivo.
    if (registro.tipo == Superior)
        posicion.ry() -= 65 * tiempo * tiempo;
    else if (registro.tipo == Inferior)
        posicion.ry() += 65 * tiempo * tiempo;
    else if (registro.tipo == Oscilatorio)
        posicion.ry() += 62 * (cos(12 * tiempo) - 1);
    setPos(posicion);
}
void ProyectilCombate::observar(const QPointF& posicionJugador)
{
    if (!enemigo || registro.observada || edad < 0.4)
        return;
    const qreal cambio = posicionJugador.y() - registro.posicionInicial.y();
    registro.respuesta = cambio < -8 ? -1 : cambio > 8 ? 1 : 0;
    registro.observada = true;
}
void ProyectilCombate::desactivar()
{
    activo = false;
    hide();
}
void ProyectilCombate::paint(QPainter* p, const QStyleOptionGraphicsItem*, QWidget*)
{
    const QColor color = registro.tipo == Oscilatorio ? QColor("#ed7cff")
        : enemigo                                     ? QColor("#ff7259")
                                                      : QColor("#40ecff");
    p->setPen(QPen(color, 2));
    p->setBrush(QColor(color.red(), color.green(), color.blue(), 95));
    p->drawEllipse(QRectF(-7, -7, 14, 14));
    p->save();
    p->rotate(qRadiansToDegrees(atan2(velocidad.y(), velocidad.x())) + 90);
    p->drawPixmap(-4, -7, sprite);
    p->restore();
}
