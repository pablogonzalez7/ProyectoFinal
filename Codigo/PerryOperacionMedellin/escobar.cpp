#include "escobar.h"
#include "recursossprites.h"
#include <QGraphicsRectItem>
#include <QTransform>
#include <QPen>
#include <QBrush>
using namespace std;

Escobar::Escobar()
    : Personaje(0)
    , cabeza(new QGraphicsRectItem(this))
{
    cabeza->setPen(Qt::NoPen);
    cabeza->setBrush(Qt::NoBrush);
    cabeza->setAcceptedMouseButtons(Qt::NoButton);
    const QPixmap hoja(":/imagenes/escobar.png");
    for (int i = 0; i < 4; ++i)
    {
        const QSize tamano = i == Incapacitado ? QSize(80, 40) : QSize(64, 72);
        fotogramas[0][i] = RecursosSprites::cargar(":/imagenes/escobar.png", tamano,
            QRect(i * hoja.width() / 4, 0, hoja.width() / 4, hoja.height()));
        fotogramas[1][i] = fotogramas[0][i].transformed(QTransform().scale(-1, 1));
    }
    setAcceptedMouseButtons(Qt::NoButton);
    setZValue(4);
    reiniciar();
}
QRectF Escobar::boundingRect() const
{
    return QRectF(0, 0, 64, 72);
}
void Escobar::actualizar(qreal) { }
void Escobar::reiniciar()
{
    pose = Sentado;
    izquierda = true;
    setOpacity(1);
    setPos(740, 388);
    show();
    actualizarSprite();
}
void Escobar::cambiarPose(Pose nuevaPose)
{
    pose = nuevaPose;
    actualizarSprite();
}
void Escobar::mirarHacia(qreal posicionX)
{
    const bool nuevaIzquierda = posicionX < x() + 32;
    if (izquierda == nuevaIzquierda)
        return;
    izquierda = nuevaIzquierda;
    actualizarSprite();
}
QPointF Escobar::origenDisparo() const
{
    return mapToScene(QPointF(izquierda ? 0 : 64, 28));
}
void Escobar::actualizarSprite()
{
    mostrarFotograma(fotogramas[izquierda ? 0 : 1][pose]);
    // En la pose de disparo la cabeza esta detras del brazo extendido.
    const qreal xCabeza = pose == Apuntando ? (izquierda ? 34 : 6) : 22;
    cabeza->setRect(xCabeza, 3, 24, 22);
}
