#ifndef JUGADOR_H
#define JUGADOR_H

#include "personaje.h"

#include <QPixmap>
#include <QPointF>

class Nivel;

class Jugador : public Personaje
{
    Q_OBJECT

public:
    explicit Jugador(const Nivel *nivelActual);

    void establecerDireccion(const QPointF &nuevaDireccion);
    void actualizar(qreal segundosTranscurridos) override;

    void alternarCamuflaje();
    void restablecer();
    bool estaCamuflado() const;
    qreal rapidezActual() const;

    void paint(QPainter *pintor,
               const QStyleOptionGraphicsItem *opciones,
               QWidget *widget) override;

signals:
    void camuflajeCambiado(bool camuflado);

private:
    enum Orientacion {
        Abajo,
        Arriba,
        Izquierda,
        Derecha
    };

    static constexpr int cantidadDirecciones = 4;
    static constexpr int cantidadFotogramas = 4;

    void cargarSprites();
    void actualizarAnimacion(qreal segundosTranscurridos, bool caminando);

    QPointF direccion;

    // Referencia al nivel; el jugador no lo elimina.
    const Nivel *nivel;

    // Los 16 fotogramas se preparan una vez y se reutilizan al dibujar.
    QPixmap fotogramas[cantidadDirecciones][cantidadFotogramas];
    QPixmap aparienciasCamufladas[cantidadDirecciones];
    bool camuflado = false;
    Orientacion orientacion = Abajo;
    int fotogramaActual = 0;
    qreal tiempoAnimacion = 0.0;
};

#endif // JUGADOR_H
