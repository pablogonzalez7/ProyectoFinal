#ifndef GUARDIAENTRADA_H
#define GUARDIAENTRADA_H
#include "personaje.h"
#include <array>
using namespace std;
class QGraphicsRectItem;

class GuardiaEntrada : public Personaje
{
public:
    enum Estado { Acercandose, Sorprendido, Atacando, Inconsciente };
    explicit GuardiaEntrada(qreal posicionInicial);
    QRectF boundingRect() const override;
    void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *) override;
    void actualizar(qreal segundos) override;
    void seguir(qreal posicionJugador, qreal segundos);
    void cambiarEstado(Estado nuevoEstado);
    void reiniciar();
    Estado obtenerEstado() const { return estado; }
    bool estaActivo() const { return estado != Inconsciente; }
    bool miraIzquierda() const { return izquierda; }
    QGraphicsRectItem *obtenerCabeza() const { return cabeza; }
private:
    void mostrarSprite();
    array<array<QPixmap,2>,2> fotogramas;
    QGraphicsRectItem *cabeza; // Item hijo: lo libera el guardia.
    qreal inicio;
    qreal tiempoAnimacion = 0;
    bool izquierda = true;
    bool caminando = false;
    Estado estado = Acercandose;
};
#endif
