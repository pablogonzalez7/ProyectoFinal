#include "obstaculo.h"
#include "recursossprites.h"
#include <QPainter>
using namespace std;
Obstaculo::Obstaculo(QGraphicsItem *padre):QGraphicsItem(padre) {
    spriteBarrera = RecursosSprites::cargar(":/imagenes/barrera.png", QSize(96,56));
    spriteLodo = RecursosSprites::cargar(":/imagenes/lodo.png", QSize(112,150));
    setAcceptedMouseButtons(Qt::NoButton);
}
QRectF Obstaculo::boundingRect() const { return tipo==Lodo ? QRectF(0,0,112,150) : QRectF(0,0,96,56); }
void Obstaculo::preparar(Tipo nuevoTipo,qreal centroX,qreal altura) {
    if(tipo!=nuevoTipo) { prepareGeometryChange(); tipo=nuevoTipo; }
    golpeado=false; setPos(centroX-boundingRect().width()/2,altura); update();
}
void Obstaculo::marcarGolpe() { golpeado=true; update(); }
void Obstaculo::paint(QPainter *p,const QStyleOptionGraphicsItem *,QWidget *) {
    p->save();
    if (tipo == Barrera && golpeado) p->setOpacity(0.45);
    p->drawPixmap(0, 0, tipo == Lodo ? spriteLodo : spriteBarrera);
    p->restore();
}
