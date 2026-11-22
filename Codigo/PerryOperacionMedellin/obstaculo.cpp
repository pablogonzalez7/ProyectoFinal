#include "obstaculo.h"
#include <QPainter>
using namespace std;
Obstaculo::Obstaculo(QGraphicsItem *padre):QGraphicsItem(padre) { setAcceptedMouseButtons(Qt::NoButton); }
QRectF Obstaculo::boundingRect() const { return tipo==Lodo ? QRectF(0,0,112,150) : QRectF(0,0,96,56); }
void Obstaculo::preparar(Tipo nuevoTipo,qreal centroX,qreal altura) {
    if(tipo!=nuevoTipo) { prepareGeometryChange(); tipo=nuevoTipo; }
    golpeado=false; setPos(centroX-boundingRect().width()/2,altura); update();
}
void Obstaculo::marcarGolpe() { golpeado=true; update(); }
void Obstaculo::paint(QPainter *p,const QStyleOptionGraphicsItem *,QWidget *) {
    p->setPen(Qt::NoPen);
    if(tipo==Lodo) {
        p->setBrush(QColor(98,65,35)); p->drawRoundedRect(boundingRect(),20,20);
        p->setBrush(QColor(139,94,49)); p->drawEllipse(QRectF(15,23,60,38));
        p->drawEllipse(QRectF(40,93,55,30));
    } else {
        p->setBrush(golpeado?QColor(93,86,77):QColor(235,136,43)); p->drawRoundedRect(boundingRect(),4,4);
        p->setBrush(QColor(240,223,185));
        for(int x=8;x+12<=96;x+=28) p->drawRect(x,12,12,28);
    }
}
