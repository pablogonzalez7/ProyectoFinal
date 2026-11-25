#ifndef ARENAPOPEYE_H
#define ARENAPOPEYE_H
#include <QGraphicsScene>
#include <QPixmap>
#include <array>
#include <vector>
#include "memoriacombate.h"
using namespace std;
class Jugador;
class Popeye;
class ProyectilCombate;
class QGraphicsEllipseItem;
class QGraphicsRectItem;
class QGraphicsLineItem;
class QGraphicsSimpleTextItem;
class QGraphicsSceneMouseEvent;
class ArenaPopeye : public QGraphicsScene
{
    Q_OBJECT
public:
    enum Estado
    {
        EnJuego,
        Victoria,
        Derrota
    };
    explicit ArenaPopeye(QObject* padre = nullptr);
    void actualizar(qreal segundos);
    void reiniciar();
    void establecerDireccion(int horizontal);
    void saltar();
    void golpear();
    void activarEscudo();
    void activarAgilidad();
    void dispararA(const QPointF& destino);
    QString informacionActual() const;
    Estado obtenerEstado() const
    {
        return estado;
    }
    Jugador* obtenerJugador() const
    {
        return jugador;
    }
    Popeye* obtenerPopeye() const
    {
        return popeye;
    }
    int corazonesRestantes() const
    {
        return corazones;
    }
    bool escudoActivo() const
    {
        return tiempoEscudo > 0;
    }
    bool agilidadActiva() const
    {
        return tiempoAgilidad > 0;
    }
    bool tieneFatiga() const
    {
        return tiempoFatiga > 0;
    }
    const array<ProyectilCombate*, 24>& obtenerBalasEnemigas() const
    {
        return balasEnemigas;
    }
    const array<ProyectilCombate*, 8>& obtenerBalasJugador() const
    {
        return balasJugador;
    }
    const vector<QRectF>& obtenerSuperficies() const
    {
        return superficies;
    }
signals:
    void informacionCambiada(const QString& texto);
    void controlesLiberados();
    void partidaPerdida();
    void combateSuperado();

protected:
    void drawBackground(QPainter*, const QRectF&) override;
    void mousePressEvent(QGraphicsSceneMouseEvent*) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent*) override;

private:
    void actualizarPaso(qreal segundos);
    void actualizarHabilidades(qreal segundos);
    void actualizarJugador(qreal segundos);
    void actualizarProyectiles(qreal segundos);
    void actualizarGolpe();
    void dispararEnemigo(TipoAtaque tipo);
    void terminarProyectil(ProyectilCombate*, ResultadoAtaque resultado);
    void recibirImpacto();
    void comprobarVictoria();
    void publicarInformacion();
    void apuntar(const QPointF& posicion);
    const vector<QRectF> superficies { { 0, 460, 960, 80 }, { 170, 375, 170, 18 }, { 390, 300, 180, 18 },
        { 610, 375, 170, 18 } };
    const vector<QPointF> destinos { { 100, 460 }, { 480, 460 }, { 860, 460 }, { 245, 375 }, { 480, 300 },
        { 695, 375 } };
    Jugador* jugador;
    Popeye* popeye;
    // Items de la escena; los arrays y el vector no son propietarios.
    array<ProyectilCombate*, 24> balasEnemigas { };
    array<ProyectilCombate*, 8> balasJugador { };
    vector<QGraphicsRectItem*> pisos;
    QGraphicsEllipseItem* escudo;
    QGraphicsRectItem* zonaGolpe;
    QGraphicsLineItem *miraHorizontal, *miraVertical;
    QGraphicsSimpleTextItem* aviso;
    QPixmap fondo;
    Estado estado = EnJuego;
    int corazones = 5, siguienteIdentificador = 0;
    qreal inmunidad = 0, esperaDisparo = 0, esperaEnemigo = 2;
    qreal tiempoEscudo = 0, recargaEscudo = 0, tiempoAgilidad = 0, tiempoFatiga = 0, recargaAgilidad = 0;
    qreal tiempoGolpe = 0, recargaGolpe = 0, siguienteEspecial = 7, avisoEspecial = 0, esperaDerrota = 0;
    bool golpeUsado = false, derrotaEmitida = false;
    int danoGolpe = 0;
    QPointF objetivoMouse { 800, 400 };
    QString ultimoTexto;
};
#endif
