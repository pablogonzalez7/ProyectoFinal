#include "liana.h"
#include "configuracionhacienda.h"
#include "recursossprites.h"
#include <QPainter>
#include <cmath>
using namespace std;

Liana::Liana(const QPointF &anclaje, qreal largo, qreal inicial)
    : longitud(largo), anguloInicial(inicial), angulo(inicial)
{
    sprite = RecursosSprites::cargar(":/imagenes/liana.png",QSize(32,qRound(longitud)));
    setPos(anclaje);
    setZValue(3);
    setAcceptedMouseButtons(Qt::NoButton);
}
QRectF Liana::boundingRect() const { return QRectF(-longitud-18,-20,2*longitud+36,longitud+48); }
void Liana::reiniciar() { angulo=anguloInicial; velocidadAngular=0; ocupada=false; update(); }
void Liana::actualizar(qreal segundos)
{
    if (!isfinite(segundos) || segundos<=0) return;
    qreal restante=qMin(segundos,qreal(0.05));
    while(restante>1e-9) {
        const qreal dt=qMin(restante,ConfiguracionHacienda::pasoMaximo);
        // Pendulo no lineal: theta'' = -(g/L) sin(theta).
        // Velocity Verlet mantiene acotado el error de energia durante el balanceo.
        const qreal aceleracion=-ConfiguracionHacienda::gravedad/longitud*sin(angulo);
        angulo+=velocidadAngular*dt+0.5*aceleracion*dt*dt;
        const qreal nuevaAceleracion=-ConfiguracionHacienda::gravedad/longitud*sin(angulo);
        velocidadAngular+=0.5*(aceleracion+nuevaAceleracion)*dt;
        restante-=dt;
    }
    update();
}
QPointF Liana::puntoAgarre() const { return pos()+QPointF(longitud*sin(angulo),longitud*cos(angulo)); }
QPointF Liana::velocidadTangencial() const
{
    return QPointF(longitud*cos(angulo)*velocidadAngular,
                  -longitud*sin(angulo)*velocidadAngular);
}
qreal Liana::anguloActual() const { return angulo; }
qreal Liana::velocidadAngularActual() const { return velocidadAngular; }
qreal Liana::longitudActual() const { return longitud; }
void Liana::establecerOcupada(bool valor) { ocupada=valor; update(); }
void Liana::paint(QPainter *p,const QStyleOptionGraphicsItem *,QWidget *)
{
    // Se gira el pintor, no se crea ni escala una imagen por fotograma.
    p->save();
    p->rotate(-angulo*180.0/acos(-1.0));
    p->drawPixmap(QPointF(-sprite.width()/2.0,0),sprite);
    if (ocupada) {
        p->setPen(QPen(QColor("#ffe49b"),2));
        p->setBrush(Qt::NoBrush);
        p->drawEllipse(QPointF(0,longitud-4),6,6);
    }
    p->restore();
}
