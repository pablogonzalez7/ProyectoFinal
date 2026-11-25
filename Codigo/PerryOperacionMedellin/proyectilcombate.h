#ifndef PROYECTILCOMBATE_H
#define PROYECTILCOMBATE_H
#include <QGraphicsItem>
#include <QPixmap>
#include "memoriacombate.h"
using namespace std;
class ProyectilCombate : public QGraphicsItem
{
public:
    ProyectilCombate();
    QRectF boundingRect() const override;
    void paint(QPainter*, const QStyleOptionGraphicsItem*, QWidget*) override;
    void activar(
        const QPointF& origen, const QPointF& destino, bool enemigo, TipoAtaque tipo, int identificador);
    void actualizar(qreal segundos, qreal factorVelocidad = 1);
    void observar(const QPointF& posicionJugador);
    void desactivar();
    bool estaActivo() const
    {
        return activo;
    }
    bool esEnemigo() const
    {
        return enemigo;
    }
    qreal edadActual() const
    {
        return edad;
    }
    const ExperienciaDisparo& experiencia() const
    {
        return registro;
    }

private:
    bool activo = false, enemigo = false;
    QPointF origen, velocidad;
    qreal tiempo = 0, edad = 0;
    ExperienciaDisparo registro;
    QPixmap sprite;
};
#endif
