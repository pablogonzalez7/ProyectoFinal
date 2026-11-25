#include "personaje.h"

#include <QPainter>
#include <QColor>

using namespace std;

Personaje::Personaje(qreal velocidadInicial, QGraphicsItem *padre)
    : QGraphicsObject(padre)
    , velocidad(velocidadInicial)
    , imagen(new QGraphicsPixmapItem(this))
{
    imagen->setAcceptedMouseButtons(Qt::NoButton);
    imagen->setShapeMode(QGraphicsPixmapItem::MaskShape);
    imagen->setFlag(QGraphicsItem::ItemStacksBehindParent);
}

QRectF Personaje::boundingRect() const
{
    // Rectángulo local que ocupa el personaje.
    return QRectF(0, 0, 32, 40);
}

void Personaje::paint(QPainter *pintor,
                      const QStyleOptionGraphicsItem *,
                      QWidget *)
{
    if (!imagen->pixmap().isNull()) return;
    // Respaldo si falta un sprite.
    pintor->setPen(Qt::NoPen);
    pintor->setBrush(QColor(0, 180, 170));
    pintor->drawRect(boundingRect());
}

QGraphicsPixmapItem *Personaje::obtenerImagen() const { return imagen; }

void Personaje::mostrarFotograma(const QPixmap &fotograma)
{
    if (imagen->pixmap().cacheKey() != fotograma.cacheKey())
        imagen->setPixmap(fotograma);
    const QRectF cuerpo = boundingRect();
    imagen->setPos(cuerpo.center().x() - fotograma.width()/2.0,
                   cuerpo.bottom() - fotograma.height());
}
