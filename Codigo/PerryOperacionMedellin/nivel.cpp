#include "nivel.h"
#include "jugador.h"
#include "llave.h"
#include "villano.h"

#include <QBrush>
#include <QColor>
#include <QFont>
#include <QGraphicsRectItem>
#include <QGraphicsSimpleTextItem>
#include <QPen>
#include <QtMath>
#include <algorithm>
#include <cmath>
#include <limits>

namespace {
qreal distanciaCuadrada(const QPointF &a, const QPointF &b)
{
    const QPointF d = a - b;
    return d.x() * d.x() + d.y() * d.y();
}

// Recorte de un segmento contra un rectangulo. t va de 0 a 1.
// Se reutiliza para paredes opacas, rayos de vision y rutas transitables.
bool cortaRectangulo(const QPointF &a, const QPointF &b,
                     const QRectF &r, qreal *entrada = nullptr)
{
    qreal minimo = 0.0;
    qreal maximo = 1.0;
    const QPointF d = b - a;
    const qreal inicios[2] = {a.x(), a.y()};
    const qreal cambios[2] = {d.x(), d.y()};
    const qreal bajos[2] = {r.left(), r.top()};
    const qreal altos[2] = {r.right(), r.bottom()};

    for (int eje = 0; eje < 2; ++eje) {
        if (qAbs(cambios[eje]) < 1.0e-9) {
            if (inicios[eje] < bajos[eje] || inicios[eje] > altos[eje]) {
                return false;
            }
        } else {
            qreal primero = (bajos[eje] - inicios[eje]) / cambios[eje];
            qreal segundo = (altos[eje] - inicios[eje]) / cambios[eje];
            if (primero > segundo) {
                std::swap(primero, segundo);
            }
            minimo = qMax(minimo, primero);
            maximo = qMin(maximo, segundo);
            if (minimo > maximo) {
                return false;
            }
        }
    }
    if (entrada) {
        *entrada = minimo;
    }
    return true;
}
}

Nivel::Nivel(QObject *padre)
    : QGraphicsScene(padre)
    , paredes{
        QRectF(0, 0, 960, 20), QRectF(0, 520, 960, 20),
        QRectF(0, 20, 20, 500), QRectF(940, 20, 20, 500),
        QRectF(300, 20, 24, 160), QRectF(300, 280, 24, 240),
        QRectF(620, 20, 24, 320), QRectF(620, 440, 24, 80),
        QRectF(20, 250, 140, 24), QRectF(240, 250, 60, 24),
        QRectF(644, 220, 96, 24), QRectF(840, 220, 100, 24)
    }
{
    setSceneRect(0, 0, ConfiguracionCasa::ancho, ConfiguracionCasa::alto);
    setBackgroundBrush(QBrush(QColor(35, 40, 45)));

    for (const QRectF &pared : paredes) {
        auto *dibujo = addRect(pared, QPen(Qt::NoPen), QBrush(QColor(125, 135, 145)));
        dibujo->setZValue(3);
    }
    prepararNavegacion();

    llaves[0] = new Llave(Llave::Casa);
    addItem(llaves[0]);
    llaves[0]->setPos(90, 430);
    llaves[1] = new Llave(Llave::Automovil);
    addItem(llaves[1]);
    llaves[1]->setPos(800, 95);
    for (Llave *llave : llaves) {
        llave->setZValue(2);
    }

    puerta = addRect(QRectF(890, 450, 40, 60), QPen(Qt::NoPen));
    auto *rotulo = addSimpleText("AL AUTO", QFont("Arial", 10));
    rotulo->setBrush(QBrush(Qt::white));
    rotulo->setPos(874, 424);

    jugador = new Jugador(this);
    addItem(jugador);
    jugador->setZValue(4);
    connect(jugador, &Jugador::camuflajeCambiado, this, &Nivel::marcarInformacion);

    const QList<QList<QPointF>> recorridos = {
        {QPointF(240, 340), QPointF(240, 470), QPointF(80, 470), QPointF(80, 340)},
        {QPointF(550, 110), QPointF(550, 440), QPointF(395, 440), QPointF(395, 110)},
        {QPointF(900, 170), QPointF(700, 170), QPointF(700, 100), QPointF(900, 100)}
    };
    guardias.reserve(recorridos.size());
    for (const QList<QPointF> &recorrido : recorridos) {
        auto *guardia = new Villano(this, jugador, recorrido);
        addItem(guardia);
        guardia->setZValue(1);
        guardias.append(guardia);
        connect(guardia, &Villano::estadoCambiado, this, &Nivel::marcarInformacion);
    }

    panelAviso = addRect(QRectF(240, 205, 480, 135), QPen(Qt::NoPen),
                         QBrush(QColor(12, 19, 25, 245)));
    panelAviso->setZValue(50);
    aviso = new QGraphicsSimpleTextItem(panelAviso);
    aviso->setBrush(QBrush(Qt::white));
    aviso->setFont(QFont("Arial", 15, QFont::Bold));
    aviso->setPos(270, 235);
    reiniciarCasa();
}

Jugador *Nivel::obtenerJugador() const { return jugador; }
const QList<Villano *> &Nivel::obtenerGuardias() const { return guardias; }
Nivel::Estado Nivel::obtenerEstado() const { return estado; }
bool Nivel::alarmaActivada() const { return alarma; }
qreal Nivel::segundosRestantes() const { return tiempoRestante; }
int Nivel::cantidadLlavesRecogidas() const { return llavesRecogidas; }
QString Nivel::informacionActual() const { return informacion; }
void Nivel::marcarInformacion() { informacionPendiente = true; }

void Nivel::reiniciarCasa()
{
    estado = EnJuego;
    alarma = false;
    llavesRecogidas = 0;
    tiempoRestante = ConfiguracionCasa::duracionAlarma;
    esperaReinicio = 0.0;
    ultimoSegundo = -1;
    for (Llave *llave : llaves) {
        llave->restablecer();
    }
    jugador->restablecer();
    jugador->setPos(100, 100);
    for (Villano *guardia : guardias) {
        guardia->restablecer();
    }
    puerta->setBrush(QBrush(QColor(170, 75, 60)));
    panelAviso->hide();
    emit controlesLiberados();
    marcarInformacion();
    actualizarIndicador();
}

void Nivel::iniciarAlarma()
{
    if (alarma) {
        return;
    }
    alarma = true;
    tiempoRestante = ConfiguracionCasa::duracionAlarma;
    for (Villano *guardia : guardias) {
        guardia->establecerAlarma(true);
    }
    puerta->setBrush(QBrush(QColor(60, 200, 100)));
    marcarInformacion();
}

void Nivel::perder(const QString &motivo)
{
    if (estado != EnJuego) {
        return;
    }
    estado = Reiniciando;
    esperaReinicio = ConfiguracionCasa::demoraReinicio;
    jugador->establecerDireccion(QPointF());
    jugador->actualizar(0.0);
    aviso->setText(motivo + "\nVolviendo al inicio...");
    panelAviso->show();
    emit controlesLiberados();
    marcarInformacion();
}

void Nivel::actualizarObjetivos()
{
    const QRectF cuerpo = jugador->sceneBoundingRect();
    for (Llave *llave : llaves) {
        if (!llave->estaRecogida() && cuerpo.intersects(llave->sceneBoundingRect())) {
            llave->recoger();
            ++llavesRecogidas;
            marcarInformacion();
        }
    }
    if (llavesRecogidas == 2) {
        iniciarAlarma();
        if (cuerpo.intersects(puerta->sceneBoundingRect())) {
            estado = Completado;
            jugador->establecerDireccion(QPointF());
            jugador->actualizar(0.0);
            aviso->setText("Llegaste al auto!\nR: volver a jugar la casa");
            panelAviso->show();
            emit controlesLiberados();
            emit casaSuperada();
            marcarInformacion();
        }
    }
}

void Nivel::actualizar(qreal segundosReales)
{
    if (!std::isfinite(segundosReales) || segundosReales <= 0.0) {
        return;
    }
    if (estado == Reiniciando) {
        esperaReinicio -= segundosReales;
        if (esperaReinicio <= 0.0) {
            reiniciarCasa();
        }
        actualizarIndicador();
        return;
    }
    if (estado == Completado) {
        return;
    }

    // El reloj usa el tiempo real; limitar la fisica no alarga la alarma.
    if (alarma) {
        tiempoRestante = qMax(qreal(0), tiempoRestante - segundosReales);
        if (tiempoRestante <= 0.0) {
            perder("Se acabo el tiempo!");
            actualizarIndicador();
            return;
        }
    }

    // Pasos cortos para detectar contacto incluso si los personajes se cruzan.
    qreal pendiente = qMin(segundosReales, qreal(0.05));
    while (pendiente > 1.0e-8 && estado == EnJuego) {
        const qreal paso = qMin(pendiente, qreal(1.0 / 120.0));
        jugador->actualizar(paso);
        for (Villano *guardia : guardias) {
            guardia->actualizar(paso);
            if (jugador->sceneBoundingRect().intersects(guardia->sceneBoundingRect())) {
                perder("Te atraparon!");
                break;
            }
        }
        if (estado == EnJuego) {
            actualizarObjetivos();
        }
        pendiente -= paso;
    }

    const int segundo = alarma ? qCeil(tiempoRestante) : -1;
    if (segundo != ultimoSegundo) {
        ultimoSegundo = segundo;
        marcarInformacion();
    }
    actualizarIndicador();
}

void Nivel::actualizarIndicador()
{
    if (!informacionPendiente) {
        return;
    }
    int perseguidores = 0;
    for (const Villano *guardia : guardias) {
        if (guardia->obtenerEstado() != Villano::Patrullando) {
            ++perseguidores;
        }
    }
    const QString modo = jugador->estaCamuflado() ? "CAMUFLAJE (lento)" : "AGENTE";
    const QString reloj = alarma ? QString("ALARMA: %1 s").arg(qCeil(tiempoRestante))
                                : "Alarma inactiva";
    QString objetivo = alarma ? "Sal por la puerta verde antes de que termine el tiempo."
                              : "Busca las llaves de la casa y del auto. Evita los conos de vision.";
    if (estado == Reiniciando) {
        objetivo = "Reiniciando la casa...";
    } else if (estado == Completado) {
        objetivo = "Casa superada. Pulsa R para volver a jugar.";
    }
    informacion = QString("Llaves: %1/2  |  %2  |  %3  |  Guardias tras Perry: %4\n%5")
        .arg(llavesRecogidas).arg(modo).arg(reloj).arg(perseguidores).arg(objetivo);
    informacionPendiente = false;
    emit informacionCambiada(informacion);
}

bool Nivel::hayLineaDeVision(const QPointF &origen, const QPointF &destino) const
{
    for (const QRectF &pared : paredes) {
        if (cortaRectangulo(origen, destino, pared)) {
            return false;
        }
    }
    return true;
}

QPointF Nivel::recortarRayo(const QPointF &origen, const QPointF &destino) const
{
    qreal limite = 1.0;
    for (const QRectF &pared : paredes) {
        qreal entrada = 0.0;
        if (cortaRectangulo(origen, destino, pared, &entrada)) {
            limite = qMin(limite, entrada);
        }
    }
    return origen + (destino - origen) * limite;
}

bool Nivel::caminoLibre(const QPointF &origen, const QPointF &destino) const
{
    // Expandir paredes equivale a mover el centro de un cuerpo de 32 x 40.
    // El margen minimo permite rozar una pared sin penetrarla.
    for (const QRectF &pared : paredes) {
        const QRectF expandida = pared.adjusted(-15.999, -19.999, 15.999, 19.999);
        if (cortaRectangulo(origen, destino, expandida)) {
            return false;
        }
    }
    return true;
}

void Nivel::prepararNavegacion()
{
    using namespace ConfiguracionCasa;
    for (int fila = 0; fila < filas; ++fila) {
        for (int columna = 0; columna < columnas; ++columna) {
            Nodo &nodo = navegacion[fila * columnas + columna];
            nodo.centro = QPointF((columna + 0.5) * tamanoCelda,
                                  (fila + 0.5) * tamanoCelda);
            const QRectF cuerpo(nodo.centro - QPointF(16, 20), QSizeF(32, 40));
            nodo.libre = sceneRect().contains(cuerpo);
            for (const QRectF &pared : paredes) {
                if (cuerpo.intersects(pared)) {
                    nodo.libre = false;
                    break;
                }
            }
        }
    }
    const int pasosFila[4] = {-1, 1, 0, 0};
    const int pasosColumna[4] = {0, 0, -1, 1};
    for (int i = 0; i < totalCeldas; ++i) {
        Nodo &nodo = navegacion[i];
        if (!nodo.libre) {
            continue;
        }
        for (int lado = 0; lado < 4; ++lado) {
            const int f = i / columnas + pasosFila[lado];
            const int c = i % columnas + pasosColumna[lado];
            if (f < 0 || f >= filas || c < 0 || c >= columnas) {
                continue;
            }
            const int vecino = f * columnas + c;
            if (navegacion[vecino].libre && caminoLibre(nodo.centro, navegacion[vecino].centro)) {
                nodo.vecinos[lado] = vecino;
            }
        }
    }
}

int Nivel::nodoMasCercano(const QPointF &punto) const
{
    int mejor = -1;
    qreal distancia = std::numeric_limits<qreal>::max();
    for (int i = 0; i < ConfiguracionCasa::totalCeldas; ++i) {
        const Nodo &nodo = navegacion[i];
        const qreal candidato = distanciaCuadrada(punto, nodo.centro);
        if (nodo.libre && candidato < distancia && caminoLibre(punto, nodo.centro)) {
            mejor = i;
            distancia = candidato;
        }
    }
    return mejor;
}

void Nivel::calcularRuta(const QPointF &origen, const QPointF &destino,
                         QList<QPointF> &resultado) const
{
    resultado.clear();
    if (caminoLibre(origen, destino)) {
        resultado.append(destino);
        return;
    }
    const int inicio = nodoMasCercano(origen);
    const int final = nodoMasCercano(destino);
    if (inicio < 0 || final < 0) {
        return;
    }

    // BFS: todas las conexiones de la rejilla tienen el mismo costo.
    std::array<int, ConfiguracionCasa::totalCeldas> anterior;
    std::array<int, ConfiguracionCasa::totalCeldas> cola;
    anterior.fill(-1);
    int lectura = 0;
    int escritura = 0;
    cola[escritura++] = inicio;
    anterior[inicio] = inicio;

    while (lectura < escritura && anterior[final] == -1) {
        const int actual = cola[lectura++];
        for (int vecino : navegacion[actual].vecinos) {
            if (vecino >= 0 && anterior[vecino] == -1) {
                anterior[vecino] = actual;
                cola[escritura++] = vecino;
            }
        }
    }
    if (anterior[final] == -1) {
        return;
    }
    int actual = final;
    while (actual != inicio) {
        resultado.append(navegacion[actual].centro);
        actual = anterior[actual];
    }
    resultado.append(navegacion[inicio].centro);
    std::reverse(resultado.begin(), resultado.end());
    resultado.append(destino);
}

QPointF Nivel::limitarDesplazamiento(const QRectF &cuerpo,
                                    const QPointF &desplazamiento) const
{
    const QRectF limites = sceneRect();
    const qreal movimientoX = desplazamiento.x();
    const qreal movimientoY = desplazamiento.y();
    qreal nuevaX = qBound(limites.left(), cuerpo.left() + movimientoX,
                         limites.right() - cuerpo.width());
    if (movimientoX != 0.0) {
        for (const QRectF &pared : paredes) {
            if (cuerpo.bottom() <= pared.top() || cuerpo.top() >= pared.bottom()) {
                continue;
            }
            if (movimientoX > 0.0 && cuerpo.right() <= pared.left()) {
                nuevaX = qMin(nuevaX, pared.left() - cuerpo.width());
            } else if (movimientoX < 0.0 && cuerpo.left() >= pared.right()) {
                nuevaX = qMax(nuevaX, pared.right());
            }
        }
    }
    QRectF horizontal = cuerpo;
    horizontal.moveLeft(nuevaX);
    qreal nuevaY = qBound(limites.top(), cuerpo.top() + movimientoY,
                         limites.bottom() - cuerpo.height());
    if (movimientoY != 0.0) {
        for (const QRectF &pared : paredes) {
            if (horizontal.right() <= pared.left() || horizontal.left() >= pared.right()) {
                continue;
            }
            if (movimientoY > 0.0 && horizontal.bottom() <= pared.top()) {
                nuevaY = qMin(nuevaY, pared.top() - cuerpo.height());
            } else if (movimientoY < 0.0 && horizontal.top() >= pared.bottom()) {
                nuevaY = qMax(nuevaY, pared.bottom());
            }
        }
    }
    return QPointF(nuevaX - cuerpo.left(), nuevaY - cuerpo.top());
}
