#include "vehiculo.h"
#include "recursossprites.h"
#include "configuracioncarretera.h"
#include <QPainter>
#include <cmath>
using namespace std;
Vehiculo::Vehiculo(QGraphicsItem *padre): QGraphicsItem(padre) {
    spriteJugador = RecursosSprites::cargar(":/imagenes/carro_perry.png", QSize(48,84));
    spriteEnemigo = RecursosSprites::cargar(":/imagenes/carro_policia.png", QSize(48,84));
    setAcceptedMouseButtons(Qt::NoButton); restablecer();
}
QRectF Vehiculo::boundingRect() const { return QRectF(0,0,48,84); }
void Vehiculo::establecerDireccion(int direccion) { direccionHorizontal=qBound(-1,direccion,1); }
void Vehiculo::restablecer() { direccionHorizontal=0; velocidad=ConfiguracionCarretera::velocidadMaxima; setPos(456,390); }
void Vehiculo::actualizar(qreal segundos,const QRectF &limites) {
    if (!isfinite(segundos)||segundos<=0) return;
    setX(qBound(limites.left(),x()+direccionHorizontal*320.0*segundos,limites.right()-48));
}
void Vehiculo::paint(QPainter *p,const QStyleOptionGraphicsItem *,QWidget *) {
    p->drawPixmap(0, 0, enemigo ? spriteEnemigo : spriteJugador);
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
