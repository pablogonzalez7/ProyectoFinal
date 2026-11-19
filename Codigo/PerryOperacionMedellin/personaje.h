#ifndef PERSONAJE_H
#define PERSONAJE_H

#include <QGraphicsObject>

class Personaje : public QGraphicsObject
{
    Q_OBJECT

public:
    explicit Personaje(qreal velocidadInicial,
                       QGraphicsItem *padre = nullptr);

    ~Personaje() override = default;

    QRectF boundingRect() const override;

    void paint(QPainter *pintor,
               const QStyleOptionGraphicsItem *opciones,
               QWidget *widget) override;

    virtual void actualizar(qreal segundosTranscurridos) = 0;

protected:
    qreal velocidad;
};

#endif // PERSONAJE_H
