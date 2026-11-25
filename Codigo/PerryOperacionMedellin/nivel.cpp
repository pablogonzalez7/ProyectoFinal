#include "nivel.h"
#include "eventossonido.h"
#include "jugador.h"
#include "llave.h"
#include "villano.h"
#include "recursossprites.h"

#include <QBrush>
#include <QColor>
#include <QFont>
#include <QGraphicsRectItem>
#include <QGraphicsPixmapItem>
#include <QImageReader>
#include <QGraphicsSimpleTextItem>
#include <QPen>
#include <QtMath>
#include <QStringList>
#include <algorithm>
#include <cmath>
#include <limits>

using namespace std;

namespace
{
qreal distanciaCuadrada(const QPointF& a, const QPointF& b)
{
    const QPointF d = a - b;
    return d.x() * d.x() + d.y() * d.y();
}

// Recorte de un segmento contra un rectangulo. t va de 0 a 1.
// Se reutiliza para paredes opacas, rayos de vision y rutas transitables.
bool cortaRectangulo(const QPointF& a, const QPointF& b, const QRectF& r, qreal* entrada = nullptr)
{
    qreal minimo = 0.0;
    qreal maximo = 1.0;
    const QPointF d = b - a;
    const qreal inicios[2] = { a.x(), a.y() };
    const qreal cambios[2] = { d.x(), d.y() };
    const qreal bajos[2] = { r.left(), r.top() };
    const qreal altos[2] = { r.right(), r.bottom() };

    for (int eje = 0; eje < 2; ++eje)
    {
        if (qAbs(cambios[eje]) < 1.0e-9)
        {
            if (inicios[eje] < bajos[eje] || inicios[eje] > altos[eje])
            {
                return false;
            }
        }
        else
        {
            qreal primero = (bajos[eje] - inicios[eje]) / cambios[eje];
            qreal segundo = (altos[eje] - inicios[eje]) / cambios[eje];
            if (primero > segundo)
            {
                swap(primero, segundo);
            }
            minimo = qMax(minimo, primero);
            maximo = qMin(maximo, segundo);
            if (minimo > maximo)
            {
                return false;
            }
        }
    }
    if (entrada)
    {
        *entrada = minimo;
    }
    return true;
}
}

Nivel::Nivel(QObject* padre)
    : QGraphicsScene(padre)
{
    // Geometria usada una sola vez para construir la escena original.
    const QList<QRectF> paredes { // Seis habitaciones. Puertas de 100 px: cabe el cuerpo completo.
        QRectF(0, 0, 1440, 20), QRectF(0, 780, 1440, 20), QRectF(0, 20, 20, 760), QRectF(1420, 20, 20, 760),
        QRectF(460, 20, 20, 240), QRectF(460, 360, 20, 260), QRectF(460, 720, 20, 60),
        QRectF(940, 20, 20, 120), QRectF(940, 240, 20, 380), QRectF(940, 720, 20, 60),
        QRectF(20, 400, 120, 20), QRectF(240, 400, 420, 20), QRectF(760, 400, 420, 20),
        QRectF(1280, 400, 140, 20),
        // Muebles opacos: se usan en colisiones, vision y navegacion.
        QRectF(260, 100, 120, 100), QRectF(60, 520, 100, 60), QRectF(300, 600, 80, 100),
        QRectF(620, 100, 160, 80), QRectF(580, 520, 120, 80), QRectF(800, 640, 60, 80),
        QRectF(1100, 160, 80, 120), QRectF(1040, 520, 100, 80), QRectF(1260, 580, 80, 80)
    };
    obstaculos.reserve(paredes.size());
    setSceneRect(0, 0, ConfiguracionCasa::ancho, ConfiguracionCasa::alto);
    setBackgroundBrush(QBrush(QColor(35, 40, 45)));

    const QStringList nombres
        = { "DORMITORIO", "SALA", "OFICINA / AUTO", "BODEGA / CASA", "COMEDOR", "GARAJE" };
    // Una celda por habitacion. Las texturas se preparan al construir la escena.
    const QString rutaPisos = ":/imagenes/pisos_casa.png";
    const QSize hojaPisos = QImageReader(rutaPisos).size();
    const int anchoCelda = hojaPisos.width() / 3;
    const int altoCelda = hojaPisos.height() / 2;
    for (int i = 0; i < 6; ++i)
    {
        const int columna = i % 3;
        const int fila = i / 3;
        auto* suelo = addPixmap(RecursosSprites::cargar(rutaPisos, QSize(480, 400),
            QRect(columna * anchoCelda, fila * altoCelda, anchoCelda, altoCelda)));
        suelo->setPos(columna * 480, fila * 400);
        suelo->setZValue(-5);
        suelo->setAcceptedMouseButtons(Qt::NoButton);
        auto* nombre = addSimpleText(nombres.at(i), QFont("Arial", 11, QFont::Bold));
        nombre->setBrush(QColor("#f4ead4"));
        nombre->setPos(35 + columna * 480, 30 + fila * 400);
        nombre->setZValue(-3);
        auto* fondoNombre = addRect(nombre->sceneBoundingRect().adjusted(-6, -3, 6, 3), QPen(Qt::NoPen),
            QBrush(QColor(25, 32, 35, 220)));
        fondoNombre->setZValue(-4);
    }

    const QString rutaMuebles = ":/imagenes/muebles_casa.png";
    const QSize hojaMuebles = QImageReader(rutaMuebles).size();
    const int anchoMueble = hojaMuebles.width() / 3;
    const int altoMueble = hojaMuebles.height() / 3;
    const int inicioMuebles = paredes.size() - 9;
    for (int i = 0; i < paredes.size(); ++i)
    {
        const QRectF& pared = paredes[i];
        if (i < inicioMuebles)
        {
            // Las paredes y sus huecos conservan exactamente la geometria fisica.
            auto* dibujo = addRect(pared, QPen(Qt::NoPen), QBrush(QColor("#879181")));
            dibujo->setZValue(3);
            obstaculos.append(dibujo);
        }
        else
        {
            const int celda = i - inicioMuebles;
            auto* mueble = addPixmap(RecursosSprites::cargar(rutaMuebles, pared.size().toSize(),
                QRect((celda % 3) * anchoMueble, (celda / 3) * altoMueble, anchoMueble, altoMueble)));
            mueble->setPos(pared.topLeft());
            mueble->setZValue(3);
            mueble->setShapeMode(QGraphicsPixmapItem::MaskShape);
            obstaculos.append(mueble);
            mueble->setAcceptedMouseButtons(Qt::NoButton);
        }
    }
    prepararNavegacion();

    llaves[0] = new Llave(Llave::Casa);
    addItem(llaves[0]);
    llaves[0]->setPos(90, 680);
    llaves[1] = new Llave(Llave::Automovil);
    addItem(llaves[1]);
    llaves[1]->setPos(1280, 95);
    for (Llave* llave : llaves)
    {
        llave->setZValue(2);
    }

    puerta = addRect(QRectF(1360, 710, 40, 60), QPen(Qt::NoPen));
    auto* rotulo = addSimpleText("AL AUTO", QFont("Arial", 10));
    rotulo->setBrush(QBrush(Qt::white));
    rotulo->setPos(1334, 684);

    jugador = new Jugador(this);
    addItem(jugador);
    jugador->setZValue(4);
    connect(jugador, SIGNAL(camuflajeCambiado(bool)), this, SLOT(marcarInformacion()));

    const QList<QList<QPointF>> recorridos
        = { { QPointF(220, 660), QPointF(60, 660), QPointF(220, 740), QPointF(420, 660) },
              { QPointF(540, 300), QPointF(860, 300), QPointF(860, 100), QPointF(540, 220) },
              { QPointF(1240, 120), QPointF(1360, 120), QPointF(1360, 340), QPointF(1240, 340) },
              { QPointF(900, 680), QPointF(1200, 680), QPointF(1200, 460), QPointF(900, 460) } };
    guardias.reserve(recorridos.size());
    for (const QList<QPointF>& recorrido : recorridos)
    {
        auto* guardia = new Villano(this, jugador, recorrido);
        addItem(guardia);
        guardia->setZValue(1);
        guardias.append(guardia);
        connect(guardia, SIGNAL(estadoCambiado()), this, SLOT(marcarInformacion()));
        connect(guardia, SIGNAL(jugadorDetectado(QPointF)), this, SLOT(compartirAvistamiento(QPointF)));
    }

    panelAviso = addRect(QRectF(0, 0, 480, 135), QPen(Qt::NoPen), QBrush(QColor(12, 19, 25, 245)));
    panelAviso->setZValue(50);
    aviso = new QGraphicsSimpleTextItem(panelAviso);
    aviso->setBrush(QBrush(Qt::white));
    aviso->setFont(QFont("Arial", 15, QFont::Bold));
    aviso->setPos(24, 25);
    reiniciarCasa();
}

Jugador* Nivel::obtenerJugador() const { return jugador; }
const QList<Villano*>& Nivel::obtenerGuardias() const { return guardias; }
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
    llaveEnProceso = -1;
    tiempoRecogida = 0.0;
    ultimoProgreso = -1;
    for (Llave* llave : llaves)
    {
        llave->restablecer();
    }
    jugador->restablecer();
    jugador->setPos(100, 100);
    for (Villano* guardia : guardias)
    {
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
    if (alarma)
    {
        return;
    }
    alarma = true;
    emit sonidoSolicitado(Sonido::Alarma);
    tiempoRestante = ConfiguracionCasa::duracionAlarma;
    for (Villano* guardia : guardias)
    {
        guardia->establecerAlarma(true);
    }
    puerta->setBrush(QBrush(QColor(60, 200, 100)));
    marcarInformacion();
}

void Nivel::perder(const QString& motivo)
{
    if (estado != EnJuego)
    {
        return;
    }
    estado = Reiniciando;
    emit capturaOAlarmaAgotada();
    emit sonidoSolicitado(Sonido::Derrota);
    esperaReinicio = ConfiguracionCasa::demoraReinicio;
    jugador->establecerDireccion(QPointF());
    jugador->actualizar(0.0);
    aviso->setText(motivo + "\nVolviendo al inicio...");
    mostrarAviso();
    emit controlesLiberados();
    marcarInformacion();
}

void Nivel::mostrarAviso()
{
    // El panel se situa en el centro de la camara, incluso lejos del inicio.
    const QPointF centro = jugador->sceneBoundingRect().center();
    const qreal x = qBound(qreal(ConfiguracionCasa::anchoVista / 2), centro.x(),
        qreal(ConfiguracionCasa::ancho - ConfiguracionCasa::anchoVista / 2));
    const qreal y = qBound(qreal(ConfiguracionCasa::altoVista / 2), centro.y(),
        qreal(ConfiguracionCasa::alto - ConfiguracionCasa::altoVista / 2));
    panelAviso->setPos(x - 240, y - 67);
    panelAviso->show();
}

void Nivel::actualizarObjetivos(qreal segundos, bool seMovio)
{
    int candidata = -1;
    if (!jugador->estaCamuflado() && !seMovio)
    {
        for (int i = 0; i < static_cast<int>(llaves.size()); ++i)
        {
            if (!llaves[i]->estaRecogida()
                && jugador->collidesWithItem(llaves[i], Qt::IntersectsItemBoundingRect))
            {
                candidata = i;
                break;
            }
        }
    }
    if (candidata != llaveEnProceso || candidata < 0)
    {
        tiempoRecogida = 0.0;
        llaveEnProceso = candidata;
    }
    if (candidata >= 0)
    {
        tiempoRecogida += segundos;
        if (tiempoRecogida >= ConfiguracionCasa::tiempoRecogerLlave)
        {
            llaves[candidata]->recoger();
            ++llavesRecogidas;
            emit sonidoSolicitado(Sonido::Llave);
            tiempoRecogida = 0.0;
            llaveEnProceso = -1;
            marcarInformacion();
        }
    }
    const int progreso = llaveEnProceso < 0
        ? -1
        : qMin(100, int(100 * tiempoRecogida / ConfiguracionCasa::tiempoRecogerLlave)) / 10;
    if (progreso != ultimoProgreso)
    {
        ultimoProgreso = progreso;
        marcarInformacion();
    }
    if (llavesRecogidas == 2)
    {
        iniciarAlarma();
        if (jugador->collidesWithItem(puerta, Qt::IntersectsItemBoundingRect))
        {
            estado = Completado;
            jugador->establecerDireccion(QPointF());
            jugador->actualizar(0.0);
            aviso->setText("Llegaste al auto!\nR: volver a jugar la casa");
            mostrarAviso();
            emit controlesLiberados();
            emit sonidoSolicitado(Sonido::Victoria);
            emit casaSuperada();
            marcarInformacion();
        }
    }
}

void Nivel::actualizar(qreal segundosReales)
{
    if (!isfinite(segundosReales) || segundosReales <= 0.0)
    {
        return;
    }
    if (estado == Reiniciando)
    {
        esperaReinicio -= segundosReales;
        if (esperaReinicio <= 0.0)
        {
            reiniciarCasa();
        }
        actualizarIndicador();
        return;
    }
    if (estado == Completado)
    {
        return;
    }

    // Duracion real de la habilidad, independiente del limite de paso fisico.
    const int camuflajeAnterior = qCeil(jugador->segundosCamuflaje());
    jugador->actualizarCamuflaje(segundosReales);
    if (camuflajeAnterior != qCeil(jugador->segundosCamuflaje()))
    {
        marcarInformacion();
    }

    // El reloj usa el tiempo real; limitar la fisica no alarga la alarma.
    if (alarma)
    {
        tiempoRestante = qMax(qreal(0), tiempoRestante - segundosReales);
        if (tiempoRestante <= 0.0)
        {
            perder("Se acabo el tiempo!");
            actualizarIndicador();
            return;
        }
    }

    // Pasos cortos para detectar contacto incluso si los personajes se cruzan.
    qreal pendiente = qMin(segundosReales, qreal(0.05));
    while (pendiente > 1.0e-8 && estado == EnJuego)
    {
        const qreal paso = qMin(pendiente, qreal(1.0 / 120.0));
        const QPointF posicionAnterior = jugador->pos();
        jugador->actualizar(paso);
        for (Villano* guardia : guardias)
        {
            guardia->actualizar(paso);
            if (!jugador->estaCamuflado()
                && jugador->collidesWithItem(guardia, Qt::IntersectsItemBoundingRect))
            {
                perder("Te atraparon!");
                break;
            }
        }
        if (estado == EnJuego)
        {
            actualizarObjetivos(paso, jugador->pos() != posicionAnterior);
        }
        pendiente -= paso;
    }

    const int segundo = alarma ? qCeil(tiempoRestante) : -1;
    if (segundo != ultimoSegundo)
    {
        ultimoSegundo = segundo;
        marcarInformacion();
    }
    actualizarIndicador();
}

void Nivel::actualizarIndicador()
{
    if (!informacionPendiente)
    {
        return;
    }
    int perseguidores = 0;
    for (const Villano* guardia : guardias)
    {
        if (guardia->obtenerEstado() != Villano::Patrullando)
        {
            ++perseguidores;
        }
    }
    const QString modo = jugador->estaCamuflado()
        ? QString("CAMUFLADO: %1 s").arg(qCeil(jugador->segundosCamuflaje()))
        : "AGENTE";
    const QString reloj = alarma ? QString("ALARMA: %1 s").arg(qCeil(tiempoRestante)) : "Alarma inactiva";
    QString objetivo = alarma ? "Sal por la puerta verde antes de que termine el tiempo."
                              : "Llaves: bodega y oficina. Quieto con sombrero 1,2 s para recoger.";
    if (estado == Reiniciando)
    {
        objetivo = "Reiniciando la casa...";
    }
    else if (estado == Completado)
    {
        objetivo = "Casa superada. Pulsa R para volver a jugar.";
    }
    if (llaveEnProceso >= 0 && estado == EnJuego)
    {
        objetivo
            = QString("Recogiendo llave: %1% | No te muevas; observa al guardia.").arg(ultimoProgreso * 10);
    }
    informacion = QString("Llaves: %1/2  |  %2  |  %3  |  Guardias tras Perry: %4\n%5")
                      .arg(llavesRecogidas)
                      .arg(modo)
                      .arg(reloj)
                      .arg(perseguidores)
                      .arg(objetivo);
    informacion.prepend(QString("[%1] ").arg(nombreDificultad(dificultad)));
    informacionPendiente = false;
    emit informacionCambiada(informacion);
}

bool Nivel::hayLineaDeVision(const QPointF& origen, const QPointF& destino) const
{
    for (QGraphicsItem* obstaculo : obstaculos)
    {
        const QRectF pared = obstaculo->sceneBoundingRect();
        if (cortaRectangulo(origen, destino, pared))
        {
            return false;
        }
    }
    return true;
}

QPointF Nivel::recortarRayo(const QPointF& origen, const QPointF& destino) const
{
    qreal limite = 1.0;
    for (QGraphicsItem* obstaculo : obstaculos)
    {
        const QRectF pared = obstaculo->sceneBoundingRect();
        qreal entrada = 0.0;
        if (cortaRectangulo(origen, destino, pared, &entrada))
        {
            limite = qMin(limite, entrada);
        }
    }
    return origen + (destino - origen) * limite;
}

bool Nivel::caminoLibre(const QPointF& origen, const QPointF& destino) const
{
    // Expandir paredes equivale a mover el centro de un cuerpo de 32 x 40.
    // El margen minimo permite rozar una pared sin penetrarla.
    for (QGraphicsItem* obstaculo : obstaculos)
    {
        const QRectF pared = obstaculo->sceneBoundingRect();
        const QRectF expandida = pared.adjusted(-15.999, -19.999, 15.999, 19.999);
        if (cortaRectangulo(origen, destino, expandida))
        {
            return false;
        }
    }
    return true;
}

void Nivel::prepararNavegacion()
{
    using namespace ConfiguracionCasa;
    for (int fila = 0; fila < filas; ++fila)
    {
        for (int columna = 0; columna < columnas; ++columna)
        {
            Nodo& nodo = navegacion[fila * columnas + columna];
            nodo.centro = QPointF((columna + 0.5) * tamanoCelda, (fila + 0.5) * tamanoCelda);
            const QRectF cuerpo(nodo.centro - QPointF(16, 20), QSizeF(32, 40));
            nodo.libre = sceneRect().contains(cuerpo);
            for (QGraphicsItem* obstaculo : obstaculos)
            {
                const QRectF pared = obstaculo->sceneBoundingRect();
                if (cuerpo.intersects(pared))
                {
                    nodo.libre = false;
                    break;
                }
            }
        }
    }
    const int pasosFila[4] = { -1, 1, 0, 0 };
    const int pasosColumna[4] = { 0, 0, -1, 1 };
    for (int i = 0; i < totalCeldas; ++i)
    {
        Nodo& nodo = navegacion[i];
        if (!nodo.libre)
        {
            continue;
        }
        for (int lado = 0; lado < 4; ++lado)
        {
            const int f = i / columnas + pasosFila[lado];
            const int c = i % columnas + pasosColumna[lado];
            if (f < 0 || f >= filas || c < 0 || c >= columnas)
            {
                continue;
            }
            const int vecino = f * columnas + c;
            if (navegacion[vecino].libre && caminoLibre(nodo.centro, navegacion[vecino].centro))
            {
                nodo.vecinos[lado] = vecino;
            }
        }
    }
}

int Nivel::nodoMasCercano(const QPointF& punto) const
{
    int mejor = -1;
    qreal distancia = numeric_limits<qreal>::max();
    for (int i = 0; i < ConfiguracionCasa::totalCeldas; ++i)
    {
        const Nodo& nodo = navegacion[i];
        const qreal candidato = distanciaCuadrada(punto, nodo.centro);
        if (nodo.libre && candidato < distancia && caminoLibre(punto, nodo.centro))
        {
            mejor = i;
            distancia = candidato;
        }
    }
    return mejor;
}

void Nivel::calcularRuta(const QPointF& origen, const QPointF& destino, QList<QPointF>& resultado) const
{
    resultado.clear();
    if (caminoLibre(origen, destino))
    {
        resultado.append(destino);
        return;
    }
    const int inicio = nodoMasCercano(origen);
    const int final = nodoMasCercano(destino);
    if (inicio < 0 || final < 0)
    {
        return;
    }

    // BFS: todas las conexiones de la rejilla tienen el mismo costo.
    array<int, ConfiguracionCasa::totalCeldas> anterior;
    array<int, ConfiguracionCasa::totalCeldas> cola;
    anterior.fill(-1);
    int lectura = 0;
    int escritura = 0;
    cola[escritura++] = inicio;
    anterior[inicio] = inicio;

    while (lectura < escritura && anterior[final] == -1)
    {
        const int actual = cola[lectura++];
        for (int vecino : navegacion[actual].vecinos)
        {
            if (vecino >= 0 && anterior[vecino] == -1)
            {
                anterior[vecino] = actual;
                cola[escritura++] = vecino;
            }
        }
    }
    if (anterior[final] == -1)
    {
        return;
    }
    int actual = final;
    while (actual != inicio)
    {
        resultado.append(navegacion[actual].centro);
        actual = anterior[actual];
    }
    resultado.append(navegacion[inicio].centro);
    reverse(resultado.begin(), resultado.end());
    resultado.append(destino);
}

const QList<QGraphicsItem*>& Nivel::obtenerObstaculos() const { return obstaculos; }

bool Nivel::hayChoque(Personaje* personaje) const
{
    for (int i = 0; i < obstaculos.size(); ++i)
    {
        // Paredes: cuerpo estable. Muebles: silueta visible de ambos sprites.
        QGraphicsItem* cuerpo = personaje;
        if (i >= cantidadParedes)
            cuerpo = personaje->obtenerImagen();
        if (cuerpo->collidesWithItem(obstaculos.at(i)))
            return true;
    }
    return false;
}

QPointF Nivel::moverPersonaje(Personaje* personaje, const QPointF& desplazamiento) const
{
    const QPointF inicio = personaje->pos();
    const int pasos = qMax(1, qCeil(qMax(qAbs(desplazamiento.x()), qAbs(desplazamiento.y())) / 4.0));
    const QPointF paso = desplazamiento / pasos;
    // Separar X e Y permite deslizarse a lo largo de una pared.
    for (int numero = 0; numero < pasos; ++numero)
    {
        for (int eje = 0; eje < 2; ++eje)
        {
            const QPointF avance = eje == 0 ? QPointF(paso.x(), 0) : QPointF(0, paso.y());
            if (avance.isNull())
                continue;
            const QPointF anterior = personaje->pos();
            personaje->setPos(anterior + avance);
            if (hayChoque(personaje))
            {
                // Acercarse al contacto sin dejar un salto visible de varios pixeles.
                qreal libre = 0.0;
                qreal ocupado = 1.0;
                for (int ajuste = 0; ajuste < 7; ++ajuste)
                {
                    const qreal medio = (libre + ocupado) / 2.0;
                    personaje->setPos(anterior + avance * medio);
                    if (hayChoque(personaje))
                        ocupado = medio;
                    else
                        libre = medio;
                }
                personaje->setPos(anterior + avance * libre);
            }
        }
    }
    return personaje->pos() - inicio;
}

void Nivel::establecerDificultad(Dificultad nueva)
{
    dificultad = nueva;
    for (Villano* guardia : guardias)
        guardia->establecerDificultad(nueva);
}
void Nivel::compartirAvistamiento(const QPointF& posicion)
{
    if (dificultad != Dificultad::Dificil)
        return;
    for (Villano* guardia : guardias)
        guardia->recibirAviso(posicion);
}
