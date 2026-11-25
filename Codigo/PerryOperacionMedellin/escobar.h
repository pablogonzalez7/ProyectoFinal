#ifndef ESCOBAR_H
#define ESCOBAR_H

#include "personaje.h"
#include <array>
using namespace std;
class QGraphicsRectItem;

class Escobar : public Personaje
{
public:
    enum Pose
    {
        Sentado,
        DePie,
        Apuntando,
        Incapacitado
    };
    Escobar();
    QRectF boundingRect() const override;
    void actualizar(qreal segundos) override;
    void cambiarPose(Pose nuevaPose);
    void mirarHacia(qreal posicionX);
    void reiniciar();
    QPointF origenDisparo() const;
    QGraphicsRectItem* obtenerCabeza() const
    {
        return cabeza;
    }
    Pose poseActual() const
    {
        return pose;
    }

private:
    void actualizarSprite();
    array<array<QPixmap, 4>, 2> fotogramas;
    QGraphicsRectItem* cabeza; // Item hijo; lo libera el personaje.
    Pose pose = Sentado;
    bool izquierda = true;
};
#endif
