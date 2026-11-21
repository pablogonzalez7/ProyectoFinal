#include "personaje.h"

#include <QPainter>
#include <QColor>

using namespace std;

Personaje::Personaje(qreal velocidadInicial, QGraphicsItem *padre)
    : QGraphicsObject(padre)
    , velocidad(velocidadInicial)
{
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
    // Representación provisional hasta incorporar el sprite.
    pintor->setPen(Qt::NoPen);
    pintor->setBrush(QColor(0, 180, 170));
    pintor->drawRect(boundingRect());
}
