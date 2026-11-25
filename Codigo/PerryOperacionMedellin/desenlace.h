#ifndef DESENLACE_H
#define DESENLACE_H

#include <QGraphicsScene>
#include <QPixmap>
#include <array>
using namespace std;
class Jugador;
class Escobar;
class Proyectil;
class QGraphicsRectItem;
class QGraphicsEllipseItem;
class QGraphicsTextItem;

class Desenlace : public QGraphicsScene
{
    Q_OBJECT
public:
    enum Estado
    {
        Dialogo,
        Eleccion,
        Advertencia,
        Combate,
        Entrega,
        Portal,
        Victoria,
        Derrota
    };
    explicit Desenlace(QObject* padre = nullptr);
    void reiniciar(int corazonesConservados);
    void actualizar(qreal segundos);
    void continuar();
    void elegir(bool aceptarTrato);
    void establecerDireccion(int direccion);
    void saltar();
    void golpear();
    void activarEscudo();
    Estado obtenerEstado() const
    {
        return estado;
    }
    int corazonesRestantes() const
    {
        return corazones;
    }
    bool escudoActivo() const
    {
        return tiempoEscudo > 0;
    }
    Jugador* obtenerJugador() const
    {
        return jugador;
    }
    Escobar* obtenerEscobar() const
    {
        return escobar;
    }
    const array<Proyectil*, 8>& obtenerProyectiles() const
    {
        return balas;
    }
    QString informacionActual() const;
signals:
    void sonidoSolicitado(int evento);
    void informacionCambiada(const QString& texto);
    void controlesLiberados();
    void juegoPerdido();
    void reinicioSolicitado();
    void juegoCompletado();

protected:
    void drawBackground(QPainter*, const QRectF&) override;

private:
    bool permiteMovimiento() const;
    void actualizarPaso(qreal segundos);
    void actualizarJugador(qreal segundos);
    void actualizarGolpe();
    void actualizarBalas(qreal segundos);
    void disparar();
    void recibirImpacto();
    void empezarEntrega();
    void empezarPortal();
    void terminar();
    void limpiarBalas();
    void publicarInformacion();
    void actualizarPanel();

    Jugador* jugador;
    Escobar* escobar;
    array<Proyectil*, 8> balas { }; // Propiedad de QGraphicsScene.
    array<QGraphicsEllipseItem*, 3> anillos { };
    QGraphicsEllipseItem* escudo;
    QGraphicsRectItem* zonaGolpe;
    QGraphicsRectItem* panel;
    QGraphicsTextItem* textoPanel;
    QPixmap fondo;
    Estado estado = Dialogo;
    int corazones = 6;
    bool aceptoTrato = false, derrotaEmitida = false, reinicioEmitido = false;
    qreal espera = 0, esperaDisparo = 0, inmunidad = 0;
    qreal tiempoEscudo = 0, recargaEscudo = 0, tiempoGolpe = 0, recargaGolpe = 0;
    qreal tiempoPortal = 0;
    QString ultimoTexto;
};
#endif
