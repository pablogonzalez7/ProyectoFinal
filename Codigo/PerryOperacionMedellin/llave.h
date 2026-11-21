#ifndef LLAVE_H
#define LLAVE_H

#include <QGraphicsItem>

using namespace std;

class Llave : public QGraphicsItem
{
public:
    enum Tipo { Casa, Automovil };

    explicit Llave(Tipo tipoLlave, QGraphicsItem *padre = nullptr);

    QRectF boundingRect() const override;
    void paint(QPainter *pintor,
               const QStyleOptionGraphicsItem *opciones,
               QWidget *widget) override;

    bool estaRecogida() const;
    void recoger();
    void restablecer();

private:
    Tipo tipo;
    bool recogida = false;
};

#endif // LLAVE_H
