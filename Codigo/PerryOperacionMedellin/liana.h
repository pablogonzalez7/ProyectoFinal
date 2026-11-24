#ifndef LIANA_H
#define LIANA_H
#include <QGraphicsItem>
#include <QPointF>
#include <QPixmap>
using namespace std;

class Liana : public QGraphicsItem
{
public:
    Liana(const QPointF &anclaje, qreal longitud, qreal anguloInicial);
    QRectF boundingRect() const override;
    void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *) override;
    void actualizar(qreal segundos);
    void reiniciar();
    QPointF puntoAgarre() const;
    QPointF velocidadTangencial() const;
    qreal anguloActual() const;
    qreal velocidadAngularActual() const;
    qreal longitudActual() const;
    void establecerOcupada(bool valor);
private:
    QPixmap sprite;
    qreal longitud;
    qreal anguloInicial;
    qreal angulo;
    qreal velocidadAngular = 0;
    bool ocupada = false;
};
#endif
