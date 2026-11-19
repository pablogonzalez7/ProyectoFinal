#include "jugador.h"
#include "nivel.h"

#include <QtMath>

Jugador::Jugador(const Nivel *nivelActual)
    : Personaje(220.0)
    , direccion(0, 0)
    , nivel(nivelActual)
{
}

void Jugador::establecerDireccion(const QPointF &nuevaDireccion)
{
    direccion = nuevaDireccion;

    const qreal longitud = qSqrt(
        direccion.x() * direccion.x()
        + direccion.y() * direccion.y()
        );

    // Mantiene la misma rapidez al caminar en diagonal.
    if (longitud > 0.0) {
        direccion /= longitud;
    }
}

void Jugador::actualizar(qreal segundosTranscurridos)
{
    if (direccion.isNull()) {
        return;
    }

    const QPointF desplazamiento =
        direccion * velocidad * segundosTranscurridos;

    // Cuerpo del jugador ubicado en las coordenadas del nivel.
    const QRectF cuerpo = boundingRect().translated(pos());

    const QPointF avancePermitido =
        nivel->limitarDesplazamiento(cuerpo, desplazamiento);

    if (!avancePermitido.isNull()) {
        setPos(pos() + avancePermitido);
    }
}
