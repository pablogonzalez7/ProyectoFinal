#ifndef HIPOPOTAMO_H
#define HIPOPOTAMO_H
#include "plataforma.h"
#include <QPixmap>
#include <array>
using namespace std;
class Hipopotamo : public Plataforma
{
public:
    Hipopotamo(qreal x, qreal y, qreal desfase = 0);
    QRectF boundingRect() const override;
    bool permiteApoyo() const override;
    void actualizar(qreal segundos) override;
    void reiniciar() override;
    void configurarCiclo(bool aleatorio, qreal desfase);
    void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *) override;
    bool estaAvisando() const;
    qreal faseActual() const;
private:
    array<QPixmap, 4> sprites;
    void cargarSprites();
    qreal fase;
    qreal desfaseInicial = 0;
    bool cicloAleatorio = true;

};
#endif
