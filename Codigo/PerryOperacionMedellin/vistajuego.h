#ifndef VISTAJUEGO_H
#define VISTAJUEGO_H
#include <QGraphicsView>
#include <QPainterPath>
// El marcador pertenece a la vista: no se desplaza con la camara ni colisiona.
class VistaJuego : public QGraphicsView
{
public:
    explicit VistaJuego(QWidget *padre = nullptr);
    void mostrarEstado(int vidasPerry, int vidasPopeye = -1, int metros = -1);
    int corazonesPerry() const { return perry; }
    int corazonesPopeye() const { return popeye; }
    int metrosMostrados() const { return distancia; }
protected:
    void drawForeground(QPainter *, const QRectF &) override;
private:
    void dibujarVidas(QPainter *, const QString &, int cantidad, qreal x);
    QPainterPath corazon;
    int perry = 1, popeye = -1, distancia = -1;
};
#endif
