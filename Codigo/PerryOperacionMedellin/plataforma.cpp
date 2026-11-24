#include "plataforma.h"
#include "recursossprites.h"
#include <QPainter>
using namespace std;
Plataforma::Plataforma(qreal x, qreal y, qreal anchura, qreal altura)
    : texturaSuelo(RecursosSprites::cargar(":/imagenes/suelo_hacienda.png",QSize(192,120))),
      ancho(anchura), alto(altura) { setPos(x, y); }
QRectF Plataforma::boundingRect() const { return QRectF(0, 0, ancho, alto); }
QRectF Plataforma::superficie() const { return QRectF(x(), y(), ancho, 0); }
bool Plataforma::permiteApoyo() const { return true; }
void Plataforma::actualizar(qreal) {}
void Plataforma::reiniciar() {}
void Plataforma::paint(QPainter *p, const QStyleOptionGraphicsItem *, QWidget *)
{
    // Mosaico compartido: no estira la tierra segun el ancho de cada orilla.
    p->drawTiledPixmap(boundingRect(),texturaSuelo);
}
