#ifndef JUGADOR_H
#define JUGADOR_H

#include "personaje.h"

#include <QPixmap>
#include <QPointF>
#include <array>

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
    void establecerDisfrazEntrada(bool activo);
    void establecerRapidezPlataformas(qreal nuevaRapidez);
    void iniciarPatada();
    bool miraIzquierda() const;
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


signals:
    void sonidoSolicitado(int evento);
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
    void mostrarSpriteActual();

    void actualizarAnimacion(qreal segundosTranscurridos, bool caminando);

    QPointF direccion;
    array<array<QPixmap, 3>, 2> fotogramasLaterales; // Izquierda/derecha: quieto, paso y salto.
    array<QPixmap,2> disfrazLateral;
    array<QPixmap,2> patadaLateral;
    bool disfrazEntrada = false;
    qreal tiempoPatada = 0;
    bool modoPlataformas = false;
    bool enSuelo = false;
    qreal velocidadVertical = 0.0;
    qreal velocidadHorizontalVuelo = 0.0;
    bool vueloInercial = false;

    // Referencia al nivel; el jugador no lo elimina.
    const Nivel *nivel;

    // Los 16 fotogramas se preparan una vez y se reutilizan al dibujar.
    array<array<QPixmap, cantidadFotogramas>, cantidadDirecciones> fotogramas;
    array<QPixmap, cantidadDirecciones> aparienciasCamufladas;
    bool camuflado = false;
    qreal tiempoCamuflaje = 0.0;
    Orientacion orientacion = Abajo;
    int fotogramaActual = 0;
    qreal tiempoAnimacion = 0.0;
};

#endif // JUGADOR_H
