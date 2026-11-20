#ifndef VILLANO_H
#define VILLANO_H

#include "personaje.h"
#include <QList>
#include <QPointF>
#include <array>

class Nivel;
class Jugador;
class QGraphicsPathItem;

// Guardia de la casa: reglas fijas, sin entrenamiento ni aprendizaje.
class Villano : public Personaje
{
    Q_OBJECT
public:
    enum Estado { Patrullando, Persiguiendo, Investigando };
    Villano(const Nivel *nivelActual, const Jugador *objetivo,
            const QList<QPointF> &puntosPatrulla);

    void actualizar(qreal segundosTranscurridos) override;
    void paint(QPainter *pintor, const QStyleOptionGraphicsItem *opciones,
               QWidget *widget) override;
    void restablecer();
    void establecerAlarma(bool activa);
    Estado obtenerEstado() const;
    bool puedeVerJugador() const;
    qreal radioDeteccion() const;

signals:
    void estadoCambiado();

private:
    QPointF centro() const;
    void cambiarEstado(Estado nuevoEstado);
    void comprobarJugador();
    void irHacia(const QPointF &destino);
    void seguirRuta(qreal segundosTranscurridos);
    void actualizarCono(bool forzar = false);

    const Nivel *nivel;
    const Jugador *jugador;
    const QList<QPointF> patrulla;
    QList<QPointF> camino;
    int indicePatrulla = 0;
    int pasoRuta = 0;
    Estado estado = Patrullando;
    bool alarma = false;
    QPointF direccionVista{1, 0};
    QPointF ultimaPosicionConocida;
    qreal esperaPercepcion = 0.0;
    qreal esperaBusqueda = 0.0;

    QGraphicsPathItem *cono;
    static constexpr int cantidadRayos = 19;
    std::array<QPointF, cantidadRayos> direccionesRayos;
    QPointF ultimaPosicionCono;
    QPointF ultimaDireccionCono;
    qreal ultimoRadioCono = -1.0;
    Estado ultimoEstadoCono = Patrullando;
    bool ultimaAlarmaCono = false;
};

#endif // VILLANO_H
