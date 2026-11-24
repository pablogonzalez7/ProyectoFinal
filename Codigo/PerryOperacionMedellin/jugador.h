#ifndef JUGADOR_H
#define JUGADOR_H

#include "personaje.h"

#include <QPixmap>
#include <QPointF>

using namespace std;

class Nivel;

class Jugador : public Personaje
{
    Q_OBJECT

public:
    explicit Jugador(const Nivel *nivelActual);

    void establecerDireccion(const QPointF &nuevaDireccion);
    void actualizar(qreal segundosTranscurridos) override;

    // La misma clase Jugador funciona en vista superior y en plataformas.
    void activarModoPlataformas();
    void saltar();
    void iniciarVuelo(const QPointF &velocidadInicial);
    QPointF velocidadVueloActual() const;
    void aterrizar(qreal alturaSuperficie);
    void establecerEnSuelo(bool apoyado);
    bool estaEnSuelo() const;
    qreal velocidadVerticalActual() const;

    void alternarCamuflaje();
    void actualizarCamuflaje(qreal segundosReales);
    qreal segundosCamuflaje() const;
    void restablecer();
    bool estaCamuflado() const;
    qreal rapidezActual() const;

    QRectF boundingRect() const override;

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
    QPixmap fotogramasLaterales[2][3]; // Izquierda/derecha: quieto, paso y salto.
    bool modoPlataformas = false;
    bool enSuelo = false;
    qreal velocidadVertical = 0.0;
    qreal velocidadHorizontalVuelo = 0.0;
    bool vueloInercial = false;

    // Referencia al nivel; el jugador no lo elimina.
    const Nivel *nivel;

    // Los 16 fotogramas se preparan una vez y se reutilizan al dibujar.
    QPixmap fotogramas[cantidadDirecciones][cantidadFotogramas];
    QPixmap aparienciasCamufladas[cantidadDirecciones];
    bool camuflado = false;
    qreal tiempoCamuflaje = 0.0;
    Orientacion orientacion = Abajo;
    int fotogramaActual = 0;
    qreal tiempoAnimacion = 0.0;
};

#endif // JUGADOR_H
