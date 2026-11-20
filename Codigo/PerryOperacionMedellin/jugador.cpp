#include "jugador.h"
#include "nivel.h"
#include "configuracion.h"

#include <QDebug>
#include <QPainter>
#include <QRect>
#include <QSize>
#include <QtMath>
#include <stdexcept>

Jugador::Jugador(const Nivel *nivelActual)
    : Personaje(ConfiguracionCasa::rapidezJugador)
    , direccion(0, 0)
    , nivel(nivelActual)
{
    cargarSprites();
}

void Jugador::cargarSprites()
{
    const QPixmap hoja(":/imagenes/perry.png");

    if (hoja.isNull()) {
        throw std::runtime_error("No se pudo cargar el recurso :/imagenes/perry.png.");
    }

    // Estos recortes corresponden a la hoja provisional del repositorio.
    if (hoja.size() != QSize(1448, 1086)) {
        throw std::runtime_error("La hoja imagenes/perry.png debe medir 1448 x 1086 pixeles.");
    }

    // Filas: abajo, arriba, izquierda y derecha.
    // La imagen tiene margenes irregulares; no se divide en celdas iguales.
    const QRect recortes[cantidadDirecciones][cantidadFotogramas] = {
        {
            QRect(244, 24, 140, 176), QRect(512, 24, 140, 176),
            QRect(794, 24, 140, 176), QRect(1042, 24, 140, 176)
        },
        {
            QRect(244, 212, 140, 176), QRect(514, 212, 140, 176),
            QRect(789, 212, 140, 176), QRect(1065, 212, 140, 176)
        },
        {
            QRect(216, 400, 208, 144), QRect(486, 400, 208, 144),
            QRect(767, 400, 208, 144), QRect(1044, 400, 208, 144)
        },
        {
            QRect(190, 558, 208, 144), QRect(470, 558, 208, 144),
            QRect(742, 558, 208, 144), QRect(1015, 558, 208, 144)
        }
    };

    const QSize tamano = boundingRect().size().toSize();

    for (int fila = 0; fila < cantidadDirecciones; ++fila) {
        for (int columna = 0; columna < cantidadFotogramas; ++columna) {
            fotogramas[fila][columna] = hoja.copy(recortes[fila][columna])
                .scaled(tamano, Qt::KeepAspectRatio, Qt::FastTransformation);
        }
    }

    // Quinta fila: cuatro apariencias sin sombrero (no es un ciclo de pasos).
    const QRect recortesCamuflados[cantidadDirecciones] = {
        QRect(244, 716, 128, 176), QRect(515, 716, 128, 176),
        QRect(750, 746, 208, 144), QRect(1023, 746, 208, 144)
    };
    for (int i = 0; i < cantidadDirecciones; ++i) {
        aparienciasCamufladas[i] = hoja.copy(recortesCamuflados[i])
            .scaled(tamano, Qt::KeepAspectRatio, Qt::FastTransformation);
    }
}

void Jugador::alternarCamuflaje()
{
    camuflado = !camuflado;
    velocidad = camuflado ? ConfiguracionCasa::rapidezCamuflado
                         : ConfiguracionCasa::rapidezJugador;
    fotogramaActual = 0;
    tiempoAnimacion = 0.0;
    update();
    emit camuflajeCambiado(camuflado);
}

void Jugador::restablecer()
{
    direccion = QPointF();
    orientacion = Abajo;
    fotogramaActual = 0;
    tiempoAnimacion = 0.0;
    const bool cambio = camuflado;
    camuflado = false;
    velocidad = ConfiguracionCasa::rapidezJugador;
    update();
    if (cambio) {
        emit camuflajeCambiado(false);
    }
}

bool Jugador::estaCamuflado() const { return camuflado; }
qreal Jugador::rapidezActual() const { return velocidad; }

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

    if (direccion.isNull()) {
        return; // Conserva hacia donde estaba mirando.
    }

    Orientacion nuevaOrientacion = orientacion;

    // En diagonal se muestra la orientacion horizontal.
    if (direccion.x() < 0.0) {
        nuevaOrientacion = Izquierda;
    } else if (direccion.x() > 0.0) {
        nuevaOrientacion = Derecha;
    } else if (direccion.y() < 0.0) {
        nuevaOrientacion = Arriba;
    } else {
        nuevaOrientacion = Abajo;
    }

    if (nuevaOrientacion != orientacion) {
        orientacion = nuevaOrientacion;
        fotogramaActual = 0;
        tiempoAnimacion = 0.0;
        update();
    }
}

void Jugador::actualizar(qreal segundosTranscurridos)
{
    if (direccion.isNull()) {
        actualizarAnimacion(segundosTranscurridos, false);
        return;
    }

    const QPointF desplazamiento =
        direccion * velocidad * segundosTranscurridos;

    const QRectF cuerpo = boundingRect().translated(pos());
    const QPointF avancePermitido =
        nivel->limitarDesplazamiento(cuerpo, desplazamiento);

    const bool caminando = !avancePermitido.isNull();

    if (caminando) {
        setPos(pos() + avancePermitido);
    }

    // Si una pared impide avanzar, tambien detiene la animacion.
    actualizarAnimacion(segundosTranscurridos, caminando);
}

void Jugador::actualizarAnimacion(qreal segundosTranscurridos, bool caminando)
{
    if (!caminando || camuflado) {
        tiempoAnimacion = 0.0;

        if (fotogramaActual != 0) {
            fotogramaActual = 0;
            update();
        }

        return;
    }

    constexpr qreal duracionFotograma = 0.12;
    tiempoAnimacion += segundosTranscurridos;

    if (tiempoAnimacion >= duracionFotograma) {
        const int pasos = static_cast<int>(tiempoAnimacion / duracionFotograma);
        tiempoAnimacion -= pasos * duracionFotograma;
        fotogramaActual = (fotogramaActual + pasos) % cantidadFotogramas;
        update();
    }
}

void Jugador::paint(QPainter *pintor,
                    const QStyleOptionGraphicsItem *opciones,
                    QWidget *widget)
{
    const QPixmap &imagen = camuflado ? aparienciasCamufladas[orientacion]
                                      : fotogramas[orientacion][fotogramaActual];

    if (imagen.isNull()) {
        // Mantiene el rectangulo visible si falta la imagen o no corresponde.
        Personaje::paint(pintor, opciones, widget);
        return;
    }

    const QRectF cuerpo = boundingRect();
    const QPointF origen(
        cuerpo.center().x() - imagen.width() / 2.0,
        cuerpo.bottom() - imagen.height()
    );

    // El dibujo cambia; la caja de colision conserva sus 32 x 40 pixeles.
    pintor->drawPixmap(origen, imagen);
}
