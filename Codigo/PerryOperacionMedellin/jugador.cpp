#include "jugador.h"
#include "eventossonido.h"
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
    mostrarSpriteActual();
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

    disfrazLateral[0] = hoja.copy(QRect(750,746,208,144))
        .scaled(QSize(56,38),Qt::KeepAspectRatio,Qt::FastTransformation);
    disfrazLateral[1] = hoja.copy(QRect(1023,746,208,144))
        .scaled(QSize(56,38),Qt::KeepAspectRatio,Qt::FastTransformation);
    patadaLateral[1] = hoja.copy(QRect(1015,895,220,168))
        .scaled(QSize(64,44),Qt::KeepAspectRatio,Qt::FastTransformation);
    patadaLateral[0] = patadaLateral[1].transformed(QTransform().scale(-1,1));

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
    mostrarSpriteActual();
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
    disfrazEntrada = false;
    tiempoPatada = 0;
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
    mostrarSpriteActual();
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
        mostrarSpriteActual();
        update();
    }
}

void Jugador::actualizar(qreal segundosTranscurridos)
{
    if (modoPlataformas) {
        if (!isfinite(segundosTranscurridos) || segundosTranscurridos <= 0) return;
        const qreal dt = segundosTranscurridos;
        if (tiempoPatada>0) {
            tiempoPatada=qMax(qreal(0),tiempoPatada-dt);
            if (tiempoPatada==0) mostrarSpriteActual();
        }
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

    const QPointF avancePermitido =
        nivel->moverPersonaje(this, desplazamiento);

    const bool caminando = !avancePermitido.isNull();


    // Si una pared impide avanzar, tambien detiene la animacion.
    actualizarAnimacion(segundosTranscurridos, caminando);
}

void Jugador::actualizarAnimacion(qreal segundosTranscurridos, bool caminando)
{
    if (!caminando || camuflado) {
        tiempoAnimacion = 0.0;

        if (fotogramaActual != 0) {
            fotogramaActual = 0;
            mostrarSpriteActual();
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
        mostrarSpriteActual();
        update();
    }
}

void Jugador::mostrarSpriteActual()
{
    const int poseLateral = !enSuelo ? 2 : (direccion.isNull() ? 0 : fotogramaActual % 2);
    const int lado = orientacion == Izquierda ? 0 : 1;
    const QPixmap &imagen = modoPlataformas
        ? (disfrazEntrada ? disfrazLateral[lado]
           : tiempoPatada>0 ? patadaLateral[lado] : fotogramasLaterales[lado][poseLateral])
        : (camuflado ? aparienciasCamufladas[orientacion] : fotogramas[orientacion][fotogramaActual]);

    mostrarFotograma(imagen);
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
    emit sonidoSolicitado(Sonido::Salto);
    enSuelo = false;
    mostrarSpriteActual();
}

void Jugador::aterrizar(qreal alturaSuperficie)
{
    setY(alturaSuperficie - boundingRect().height());
    velocidadVertical = 0;
    velocidadHorizontalVuelo = 0;
    vueloInercial = false;
    enSuelo = true;
    mostrarSpriteActual();
}

void Jugador::establecerEnSuelo(bool apoyado) { enSuelo = apoyado; mostrarSpriteActual(); }
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
    mostrarSpriteActual();
}
QPointF Jugador::velocidadVueloActual() const
{
    return QPointF(vueloInercial ? velocidadHorizontalVuelo : direccion.x()*velocidad,
                   velocidadVertical);
}

void Jugador::establecerDisfrazEntrada(bool activo)
{
    if (!modoPlataformas) return;
    disfrazEntrada=activo;
    velocidad=activo ? 150 : ConfiguracionHacienda::rapidezJugador;
    mostrarSpriteActual();
}
void Jugador::iniciarPatada()
{
    if (!modoPlataformas || enSuelo || disfrazEntrada) return;
    tiempoPatada=0.20;
    mostrarSpriteActual();
}
bool Jugador::miraIzquierda() const { return orientacion==Izquierda; }

void Jugador::establecerRapidezPlataformas(qreal nuevaRapidez)
{
    if (modoPlataformas) velocidad=qBound(qreal(100),nuevaRapidez,qreal(400));
}
