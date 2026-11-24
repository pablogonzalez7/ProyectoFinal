#include "proyectil.h"
#include "recursossprites.h"
#include <QPainter>
#include <QtMath>
using namespace std;
Proyectil::Proyectil(QGraphicsItem *padre):QGraphicsItem(padre) { sprite = RecursosSprites::cargar(":/imagenes/bala.png", QSize(8,14)); setZValue(4); setAcceptedMouseButtons(Qt::NoButton); hide(); }
QRectF Proyectil::boundingRect() const { return QRectF(-4,-7,8,14); }
void Proyectil::paint(QPainter *p,const QStyleOptionGraphicsItem *,QWidget *) {
    p->drawPixmap(-4, -7, sprite);
}
void Proyectil::activar(const QPointF &origen,const QPointF &destino) {
    const QPointF delta=destino-origen;
    const qreal longitud=qSqrt(QPointF::dotProduct(delta,delta));
    velocidad=longitud>0 ? delta*(300.0/longitud) : QPointF(0,-300);
    setPos(origen); activo=true; show();
}
void Proyectil::actualizar(qreal segundos) { if(activo) moveBy(velocidad.x()*segundos,velocidad.y()*segundos); }
void Proyectil::desactivar() { activo=false; hide(); }
