#ifndef PROYECTIL_H
#define PROYECTIL_H
#include <QGraphicsItem>
#include <QPixmap>
#include <QPointF>
using namespace std;
class Proyectil : public QGraphicsItem
{
public:
    explicit Proyectil(QGraphicsItem* padre = nullptr);
    QRectF boundingRect() const override;
    void paint(QPainter*, const QStyleOptionGraphicsItem*, QWidget*) override;
    void activar(const QPointF& origen, const QPointF& destino, qreal rapidez = 300);
    void actualizar(qreal segundos);
    void desactivar();
    bool estaActivo() const
    {
        return activo;
    }

private:
    QPixmap sprite;
    bool activo = false;
    QPointF velocidad;
};
#endif
