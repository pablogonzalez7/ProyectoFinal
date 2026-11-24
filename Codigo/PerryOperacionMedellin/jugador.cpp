#include "jugador.h"
#include "nivel.h"
#include "configuracion.h"
#include "configuracionhacienda.h"

#include <QDebug>
#include <QPainter>
#include <QRect>
#include <QSize>
#include <QTransform>
#include <QtMath>
#include <stdexcept>
#include <cmath>

using namespace std;

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
        throw runtime_error("No se pudo cargar el recurso :/imagenes/perry.png.");
    }

    // Estos recortes corresponden a la hoja provisional del repositorio.
    if (hoja.size() != QSize(1448, 1086)) {
        throw runtime_error("La hoja imagenes/perry.png debe medir 1448 x 1086 pixeles.");
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

    // Ultima fila de la misma hoja: poses laterales de pie y salto.
    // Se preparan una vez; no se recorta ni escala dentro de paint().
    const QRect laterales[3] = {
        QRect(194, 916, 210, 146), QRect(467, 914, 210, 146), QRect(745, 895, 215, 160)
    };
    for (int i = 0; i < 3; ++i) {
        fotogramasLaterales[1][i] = hoja.copy(laterales[i])
            .scaled(QSize(56, 44), Qt::KeepAspectRatio, Qt::FastTransformation);
        fotogramasLaterales[0][i] = fotogramasLaterales[1][i]
            .transformed(QTransform().scale(-1, 1));
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
    if (modoPlataformas) return;
    camuflado = !camuflado;
    tiempoCamuflaje = camuflado ? ConfiguracionCasa::duracionCamuflaje : 0.0;
    velocidad = camuflado ? ConfiguracionCasa::rapidezCamuflado
                         : ConfiguracionCasa::rapidezJugador;
    fotogramaActual = 0;
    tiempoAnimacion = 0.0;
    update();
    emit camuflajeCambiado(camuflado);
}

void Jugador::actualizarCamuflaje(qreal segundosReales)
{
    if (!camuflado || !isfinite(segundosReales) || segundosReales <= 0.0) {
        return;
    }
    tiempoCamuflaje = qMax(qreal(0), tiempoCamuflaje - segundosReales);
    if (tiempoCamuflaje <= 1.0e-8) {
        alternarCamuflaje();
    }
}

qreal Jugador::segundosCamuflaje() const { return tiempoCamuflaje; }

void Jugador::restablecer()
{
    direccion = QPointF();
    velocidadVertical = 0;
    velocidadHorizontalVuelo = 0;
    vueloInercial = false;
    enSuelo = false;
    orientacion = modoPlataformas ? Derecha : Abajo;
    fotogramaActual = 0;
    tiempoAnimacion = 0.0;
    const bool cambio = camuflado;
    camuflado = false;
    tiempoCamuflaje = 0.0;
    velocidad = modoPlataformas ? ConfiguracionHacienda::rapidezJugador : ConfiguracionCasa::rapidezJugador;
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
    if (modoPlataformas) {
        if (!isfinite(segundosTranscurridos) || segundosTranscurridos <= 0) return;
        const qreal dt = segundosTranscurridos;
        const qreal dx = (vueloInercial ? velocidadHorizontalVuelo : direccion.x() * velocidad) * dt;
        // Integracion exacta para aceleracion gravitacional constante.
        const qreal dy = enSuelo ? 0 : velocidadVertical * dt
                                  + 0.5 * ConfiguracionHacienda::gravedad * dt * dt;
        if (!enSuelo) velocidadVertical += ConfiguracionHacienda::gravedad * dt;
        moveBy(dx, dy);
        actualizarAnimacion(dt, enSuelo && abs(dx) > 0);
        return;
    }
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
    const int poseLateral = !enSuelo ? 2 : (direccion.isNull() ? 0 : fotogramaActual % 2);
    const QPixmap &imagen = modoPlataformas ? fotogramasLaterales[orientacion == Izquierda ? 0 : 1][poseLateral]
                            : camuflado ? aparienciasCamufladas[orientacion]
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

    // Cada modo conserva su caja de colision aunque cambie la pose.
    pintor->drawPixmap(origen, imagen);
}

void Jugador::activarModoPlataformas()
{
    prepareGeometryChange();
    modoPlataformas = true;
    restablecer();
}

void Jugador::saltar()
{
    if (!modoPlataformas || !enSuelo) return;
    vueloInercial = false;
    velocidadVertical = -ConfiguracionHacienda::impulsoSalto;
    enSuelo = false;
}

void Jugador::aterrizar(qreal alturaSuperficie)
{
    setY(alturaSuperficie - boundingRect().height());
    velocidadVertical = 0;
    velocidadHorizontalVuelo = 0;
    vueloInercial = false;
    enSuelo = true;
}

void Jugador::establecerEnSuelo(bool apoyado) { enSuelo = apoyado; }
bool Jugador::estaEnSuelo() const { return enSuelo; }
qreal Jugador::velocidadVerticalActual() const { return velocidadVertical; }

QRectF Jugador::boundingRect() const
{
    return modoPlataformas ? QRectF(0, 0, 56, 44) : Personaje::boundingRect();
}

void Jugador::iniciarVuelo(const QPointF &velocidadInicial)
{
    vueloInercial = true;
    enSuelo = false;
    velocidadHorizontalVuelo = velocidadInicial.x();
    velocidadVertical = velocidadInicial.y();
    if (velocidadInicial.x() != 0) orientacion = velocidadInicial.x() < 0 ? Izquierda : Derecha;
}
QPointF Jugador::velocidadVueloActual() const
{
    return QPointF(vueloInercial ? velocidadHorizontalVuelo : direccion.x()*velocidad,
                   velocidadVertical);
}
