#include "llave.h"

#include <QColor>
#include <QPainter>
#include <QPen>

Llave::Llave(Tipo tipoLlave, QGraphicsItem *padre)
    : QGraphicsItem(padre), tipo(tipoLlave)
{
    setAcceptedMouseButtons(Qt::NoButton);
}

QRectF Llave::boundingRect() const
{
    return QRectF(0, 0, 36, 22);
}

void Llave::paint(QPainter *pintor,
                  const QStyleOptionGraphicsItem *, QWidget *)
{
    const QColor color = tipo == Casa
        ? QColor(255, 205, 60) : QColor(80, 190, 255);

    pintor->setPen(QPen(color, 3));
    pintor->setBrush(Qt::NoBrush);
    pintor->drawEllipse(QRectF(2, 2, 14, 14));
    pintor->drawLine(QPointF(16, 9), QPointF(33, 9));
    pintor->drawLine(QPointF(26, 9), QPointF(26, 17));
    pintor->drawLine(QPointF(32, 9), QPointF(32, 17));
}

bool Llave::estaRecogida() const
{
    return recogida;
}

void Llave::recoger()
{
    recogida = true;
    hide();
}

void Llave::restablecer()
{
    recogida = false;
    show();
}
