#ifndef NIVEL_H
#define NIVEL_H

#include "configuracion.h"
#include "dificultad.h"
#include <QGraphicsScene>
#include <QList>
#include <QPointF>
#include <QRectF>
#include <QString>
#include <array>

using namespace std;

class Jugador;
class Personaje;
class Llave;
class Villano;
class QGraphicsRectItem;
class QGraphicsSimpleTextItem;

class Nivel : public QGraphicsScene
{
    Q_OBJECT
public:
    enum Estado
    {
        EnJuego,
        Reiniciando,
        Completado
    };
    explicit Nivel(QObject* padre = nullptr);

    void actualizar(qreal segundosReales);
    void reiniciarCasa();
    void establecerDificultad(Dificultad nueva);
    Dificultad dificultadActual() const { return dificultad; }
    Jugador* obtenerJugador() const;
    const QList<Villano*>& obtenerGuardias() const;
    Estado obtenerEstado() const;
    bool alarmaActivada() const;
    qreal segundosRestantes() const;
    int cantidadLlavesRecogidas() const;
    QString informacionActual() const;

    const QList<QGraphicsItem*>& obtenerObstaculos() const;
    QPointF moverPersonaje(Personaje* personaje, const QPointF& desplazamiento) const;
    bool hayLineaDeVision(const QPointF& origen, const QPointF& destino) const;
    QPointF recortarRayo(const QPointF& origen, const QPointF& destino) const;
    bool caminoLibre(const QPointF& origen, const QPointF& destino) const;
    void calcularRuta(const QPointF& origen, const QPointF& destino, QList<QPointF>& resultado) const;

signals:
    void informacionCambiada(const QString& texto);
    void controlesLiberados();
    void casaSuperada();
    void capturaOAlarmaAgotada();
    void sonidoSolicitado(int evento);

private slots:
    void marcarInformacion();
    void compartirAvistamiento(const QPointF& posicion);

private:
    struct Nodo
    {
        QPointF centro;
        bool libre = false;
        array<int, 4> vecinos { { -1, -1, -1, -1 } };
    };

    void prepararNavegacion();
    int nodoMasCercano(const QPointF& punto) const;
    void actualizarObjetivos(qreal segundos, bool seMovio);
    void mostrarAviso();
    void actualizarIndicador();
    void iniciarAlarma();
    void perder(const QString& motivo);

    bool hayChoque(Personaje* personaje) const;
    // Apuntadores no propietarios: la escena libera todos estos objetos.
    QList<QGraphicsItem*> obstaculos;
    static constexpr int cantidadParedes = 14;
    array<Nodo, ConfiguracionCasa::totalCeldas> navegacion;
    array<Llave*, 2> llaves { };
    QList<Villano*> guardias;
    Jugador* jugador = nullptr;
    QGraphicsRectItem* puerta = nullptr;
    QGraphicsRectItem* panelAviso = nullptr;
    QGraphicsSimpleTextItem* aviso = nullptr;
    Estado estado = EnJuego;
    bool alarma = false;
    Dificultad dificultad = Dificultad::Normal;
    int llavesRecogidas = 0;
    int llaveEnProceso = -1;
    qreal tiempoRecogida = 0.0;
    int ultimoProgreso = -1;
    qreal tiempoRestante = ConfiguracionCasa::duracionAlarma;
    qreal esperaReinicio = 0.0;
    int ultimoSegundo = -1;
    bool informacionPendiente = true;
    QString informacion;
};

#endif // NIVEL_H
