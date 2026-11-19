#include "nivel.h"

#include <QBrush>
#include <QColor>
#include <QPen>
#include <QtGlobal>

Nivel::Nivel(QObject *padre)
    : QGraphicsScene(padre)
    , paredes{
          // Bordes exteriores: x, y, ancho, alto.
          QRectF(0, 0, 960, 20),
          QRectF(0, 520, 960, 20),
          QRectF(0, 20, 20, 500),
          QRectF(940, 20, 20, 500),

          // Primera division vertical, con un paso.
          QRectF(300, 20, 24, 160),
          QRectF(300, 280, 24, 240),

          // Segunda division vertical, con otro paso.
          QRectF(620, 20, 24, 320),
          QRectF(620, 440, 24, 80),

          // Habitaciones del lado izquierdo.
          QRectF(20, 250, 140, 24),
          QRectF(240, 250, 60, 24),

          // Habitaciones del lado derecho.
          QRectF(644, 220, 96, 24),
          QRectF(840, 220, 100, 24)
      }
{
    setSceneRect(0, 0, 960, 540);
    setBackgroundBrush(QBrush(QColor(35, 40, 45)));

    const QPen borde(Qt::NoPen);
    const QBrush relleno(QColor(125, 135, 145));

    // Las paredes graficas se crean una sola vez.
    for (const QRectF &pared : paredes) {
        addRect(pared, borde, relleno);
    }
}

QPointF Nivel::limitarDesplazamiento(
    const QRectF &cuerpo,
    const QPointF &desplazamiento
    ) const
{
    const QRectF limites = sceneRect();

    const qreal movimientoX = desplazamiento.x();
    const qreal movimientoY = desplazamiento.y();

    // Primero resolvemos el movimiento horizontal.
    qreal nuevaX = qBound(
        limites.left(),
        cuerpo.left() + movimientoX,
        limites.right() - cuerpo.width()
        );

    if (movimientoX != 0.0) {
        for (const QRectF &pared : paredes) {

            // Si no coinciden verticalmente, no pueden chocar
            // durante este movimiento horizontal.
            if (cuerpo.bottom() <= pared.top()
                || cuerpo.top() >= pared.bottom()) {
                continue;
            }

            if (movimientoX > 0.0
                && cuerpo.right() <= pared.left()) {

                nuevaX = qMin(
                    nuevaX,
                    pared.left() - cuerpo.width()
                    );

            } else if (movimientoX < 0.0
                       && cuerpo.left() >= pared.right()) {

                nuevaX = qMax(nuevaX, pared.right());
            }
        }
    }

    // Usamos la posicion horizontal ya corregida.
    QRectF cuerpoHorizontal = cuerpo;
    cuerpoHorizontal.moveLeft(nuevaX);

    // Ahora resolvemos el movimiento vertical.
    qreal nuevaY = qBound(
        limites.top(),
        cuerpo.top() + movimientoY,
        limites.bottom() - cuerpo.height()
        );

    if (movimientoY != 0.0) {
        for (const QRectF &pared : paredes) {

            // Si no coinciden horizontalmente, no pueden chocar
            // durante este movimiento vertical.
            if (cuerpoHorizontal.right() <= pared.left()
                || cuerpoHorizontal.left() >= pared.right()) {
                continue;
            }

            if (movimientoY > 0.0
                && cuerpoHorizontal.bottom() <= pared.top()) {

                nuevaY = qMin(
                    nuevaY,
                    pared.top() - cuerpo.height()
                    );

            } else if (movimientoY < 0.0
                       && cuerpoHorizontal.top() >= pared.bottom()) {

                nuevaY = qMax(nuevaY, pared.bottom());
            }
        }
    }

    // Devolvemos cuanto puede avanzar realmente.
    return QPointF(
        nuevaX - cuerpo.left(),
        nuevaY - cuerpo.top()
        );
}
