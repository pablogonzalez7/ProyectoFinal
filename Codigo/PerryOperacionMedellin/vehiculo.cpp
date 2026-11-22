#include "vehiculo.h"
#include "configuracioncarretera.h"
#include <QPainter>
#include <cmath>
using namespace std;
Vehiculo::Vehiculo(QGraphicsItem *padre): QGraphicsItem(padre) { setAcceptedMouseButtons(Qt::NoButton); restablecer(); }
QRectF Vehiculo::boundingRect() const { return QRectF(0,0,48,84); }
void Vehiculo::establecerDireccion(int direccion) { direccionHorizontal=qBound(-1,direccion,1); }
void Vehiculo::restablecer() { direccionHorizontal=0; velocidad=ConfiguracionCarretera::velocidadMaxima; setPos(456,390); }
void Vehiculo::actualizar(qreal segundos,const QRectF &limites) {
    if (!isfinite(segundos)||segundos<=0) return;
    setX(qBound(limites.left(),x()+direccionHorizontal*320.0*segundos,limites.right()-48));
}
void Vehiculo::paint(QPainter *p,const QStyleOptionGraphicsItem *,QWidget *) {
    p->setPen(Qt::NoPen); p->setBrush(QColor(12,17,22));
    p->drawRect(0,14,8,18); p->drawRect(40,14,8,18);
    p->drawRect(0,56,8,18); p->drawRect(40,56,8,18);
    p->setBrush(enemigo ? QColor(175,52,49) : QColor(22,174,157)); p->drawRoundedRect(QRectF(5,0,38,84),8,8);
    p->setBrush(QColor(25,52,65)); p->drawRoundedRect(QRectF(10,19,28,22),3,3); p->drawRect(11,59,26,12);
    p->setBrush(QColor(255,237,170)); p->drawRect(8,4,9,5); p->drawRect(31,4,9,5);
    p->setBrush(QColor(239,78,60)); p->drawRect(8,76,8,5); p->drawRect(32,76,8,5);
}

qreal Vehiculo::rapidezAvance() const { return velocidad*ConfiguracionCarretera::pixelesPorMetro; }
void Vehiculo::actualizarVelocidad(qreal segundos,bool enLodo) {
    if (!isfinite(segundos)||segundos<=0) return;
    using namespace ConfiguracionCarretera;
    const qreal coeficiente=enLodo ? rozamientoLodo : rozamientoSeco;
    const qreal fuerzaRozamiento=coeficiente*masa*gravedad;
    const qreal aceleracion=(fuerzaMotor-fuerzaRozamiento)/masa;
    // v(t+dt)=v(t)+a*dt. Los limites inferior y superior son decisiones de juego.
    velocidad=qBound(velocidadMinima,velocidad+aceleracion*segundos,velocidadMaxima);
}
void Vehiculo::recibirImpacto() {
    velocidad=qMax(ConfiguracionCarretera::velocidadMinima,velocidad*0.55);
}
