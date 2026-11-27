#ifndef CARRETERA_H
#define CARRETERA_H
#include <QGraphicsScene>
#include "dificultad.h"
#include <QString>
#include <QPixmap>
#include <QRandomGenerator>
#include <array>
#include "configuracioncarretera.h"
using namespace std;
class Vehiculo;
class Obstaculo;
class Proyectil;
class QGraphicsEllipseItem;
class QGraphicsPixmapItem;
class QGraphicsLineItem;
class QGraphicsSimpleTextItem;
class Carretera : public QGraphicsScene {
    Q_OBJECT
public:
    enum Estado { Conduciendo, Derrota, Llegada };
    void activarEscudo();
    void activarDimensionador();
    Estado obtenerEstado() const { return estado; }
    int resistenciaActual() const { return resistencia; }
    qreal distanciaPerseguidor() const { return separacion; }
    bool escudoActivo() const { return tiempoEscudo>0; }
    const array<Proyectil *,ConfiguracionCarretera::cantidadProyectiles> &obtenerProyectiles() const { return proyectiles; }
    explicit Carretera(QObject *padre=nullptr);
    void actualizar(qreal segundos);
    void establecerDireccion(int direccion);
    void reiniciar();
    void establecerDificultad(Dificultad nueva);
    Dificultad dificultadActual() const { return dificultad; }
    QString informacionActual() const;
    qreal distanciaRecorrida() const { return distancia; }
    Vehiculo *obtenerVehiculo() const { return vehiculo; }
    const array<Obstaculo *,ConfiguracionCarretera::cantidadObstaculos> &obtenerObstaculos() const { return obstaculos; }
    int cantidadPerseguidores() const { return dificultad == Dificultad::Dificil ? 2 : 1; }
    int cantidadChoques() const { return choques; }
    bool estaEnLodo() const { return enLodo; }
signals:
    void sonidoSolicitado(int evento);
    void partidaPerdida();
    void haciendaAlcanzada();
    void entradaHabilitada();
    void informacionCambiada(const QString &texto);
protected:
    void drawBackground(QPainter *, const QRectF &) override;
private:
    Dificultad dificultad = Dificultad::Normal;
    Vehiculo *vehiculo; // Lo libera la escena.
    QRectF calzada{240,0,480,540};
    qreal desplazamiento=0, distancia=0;
    QPixmap texturaAsfalto, texturaLlano;
    qreal desplazamientoTextura=0;
    int ultimoMetro=-1;
    array<Obstaculo *,ConfiguracionCarretera::cantidadObstaculos> obstaculos{};
    QRandomGenerator aleatorio{21};
    int siguienteTipo=0;
    int choques=0;
    bool enLodo=false;
    QString ultimoIndicador;
    Estado estado=Conduciendo;
    Vehiculo *perseguidor=nullptr;
    Vehiculo *segundoPerseguidor=nullptr;
    QGraphicsLineItem *segundoAviso=nullptr;
    qreal segundoDisparo=4;
    QPointF segundoDestino;
    array<QPixmap,2> spritesEdificios;
    array<QGraphicsPixmapItem *,6> edificios{};
    void colocarEdificio(int indice, qreal altura);
    void actualizarEdificios(qreal avance);
    void actualizarDisparo(Vehiculo *, QGraphicsLineItem *, qreal &, QPointF &, qreal);
    QGraphicsEllipseItem *dibujoEscudo=nullptr;
    QGraphicsLineItem *avisoDisparo=nullptr;
    QGraphicsSimpleTextItem *avisoFinal=nullptr;
    array<Proyectil *,ConfiguracionCarretera::cantidadProyectiles> proyectiles{};
    qreal separacion=35, tiempoEscudo=0, recarga=0, inmunidad=0;
    qreal siguienteDisparo=3, esperaReinicio=0;
    int resistencia=3;
    bool dimensionador=false, reinicioEmitido=false;
    QPointF destinoDisparo;
    void actualizarPersecucion(qreal paso);
    void finalizar(bool victoria,const QString &mensaje);
    void publicarInformacion();
    void colocarObstaculo(Obstaculo *objeto,qreal altura);
};
#endif
