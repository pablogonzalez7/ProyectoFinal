#ifndef PLATAFORMA_H
#define PLATAFORMA_H
#include <QGraphicsItem>
#include <QPixmap>
using namespace std;

// Clase concreta: representa las orillas fijas. Hipopotamo especializa su apoyo.
class Plataforma : public QGraphicsItem
{
public:
    Plataforma(qreal x, qreal y, qreal ancho, qreal alto = 120);
    QRectF boundingRect() const override;
    void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *) override;
    virtual bool permiteApoyo() const;
    virtual void actualizar(qreal segundos);
    virtual void reiniciar();
    QRectF superficie() const;
private:
    QPixmap texturaSuelo;
protected:
    qreal ancho;
    qreal alto;
};
#endif
