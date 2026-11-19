#ifndef JUGADOR_H
#define JUGADOR_H

#include "personaje.h"

#include <QPointF>

class Nivel;

class Jugador : public Personaje
{
    Q_OBJECT

public:
    explicit Jugador(const Nivel *nivelActual);

    void establecerDireccion(const QPointF &nuevaDireccion);
    void actualizar(qreal segundosTranscurridos) override;

private:
    QPointF direccion;

    // Referencia al nivel; el jugador no lo elimina.
    const Nivel *nivel;
};

#endif // JUGADOR_H
