#include "vistajuego.h"
#include <QPainter>
#include <QFont>
VistaJuego::VistaJuego(QWidget *padre) : QGraphicsView(padre)
{
    // La silueta se prepara una sola vez y se reutiliza para cada vida.
    corazon.moveTo(11, 21);
    corazon.cubicTo(8, 17, 0, 12, 0, 6);
    corazon.cubicTo(0, -1, 8, -2, 11, 4);
    corazon.cubicTo(14, -2, 22, -1, 22, 6);
    corazon.cubicTo(22, 12, 14, 17, 11, 21);
    corazon.closeSubpath();
}
void VistaJuego::mostrarEstado(int vidasPerry, int vidasPopeye, int metros)
{
    if (perry == vidasPerry && popeye == vidasPopeye && distancia == metros) return;
    perry = qMax(0, vidasPerry);
    popeye = vidasPopeye;
    distancia = metros;
    viewport()->update();
}
void VistaJuego::dibujarVidas(QPainter *p, const QString &nombre, int cantidad, qreal x)
{
    const qreal anchoNombre = nombre == "POPEYE:" ? 105 : 90;
    p->setPen(Qt::NoPen);
    p->setBrush(QColor(12, 22, 30, 195));
    p->drawRoundedRect(QRectF(x, 12, anchoNombre + qMax(1, cantidad)*26 + 14, 42), 9, 9);
    p->setPen(Qt::white);
    QFont letra("Arial"); letra.setPixelSize(20); letra.setBold(true); p->setFont(letra);
    p->drawText(QRectF(x+10, 12, anchoNombre, 42), Qt::AlignVCenter, nombre);
    for (int i=0; i<cantidad; ++i) {
        p->save(); p->translate(x+anchoNombre+i*26, 23);
        p->setPen(QPen(QColor("#7b1024"), 1.5)); p->setBrush(QColor("#ff2948"));
        p->drawPath(corazon);
        p->setPen(QPen(QColor("#ff9aaa"), 2)); p->drawLine(4,5,6,3);
        p->restore();
    }
}
void VistaJuego::drawForeground(QPainter *p, const QRectF &)
{
    p->save(); p->resetTransform(); p->setRenderHint(QPainter::Antialiasing);
    dibujarVidas(p, "PERRY:", perry, 12);
    if (popeye >= 0)
        dibujarVidas(p, "POPEYE:", popeye, viewport()->width()-12-105-qMax(1,popeye)*26-14);
    if (distancia >= 0) {
        const QString texto = QString("%1 mts / 1200 mts").arg(distancia);
        QFont letra("Arial"); letra.setPixelSize(28); letra.setBold(true); p->setFont(letra);
        p->setPen(Qt::NoPen); p->setBrush(QColor(12,22,30,195));
        p->drawRoundedRect(QRectF(12, 62, 345, 44), 8, 8);
        p->setPen(Qt::white); p->drawText(QRectF(24,62,325,44),Qt::AlignVCenter,texto);
    }
    p->restore();
}
