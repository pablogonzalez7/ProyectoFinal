#ifndef ENTRADAHACIENDA_H
#define ENTRADAHACIENDA_H
#include <QGraphicsScene>
#include "dificultad.h"
#include <QPixmap>
#include <array>
using namespace std;
class Jugador;
class GuardiaEntrada;
class Proyectil;
class QGraphicsRectItem;
class EntradaHacienda : public QGraphicsScene
{
    Q_OBJECT
public:
    enum Estado { Disfrazado, Sorpresa, Combate, Despejada, Derrota, Completada };
    explicit EntradaHacienda(QObject *padre=nullptr);
    void actualizar(qreal segundos);
    void establecerDireccion(int horizontal);
    void saltar();
    void transformar();
    void golpear();
    void reiniciar();
    void establecerDificultad(Dificultad nueva);
    Dificultad dificultadActual() const { return dificultad; }
    QString informacionActual() const;
    Estado obtenerEstado() const { return estado; }
    int numeroGuardias() const { return cantidadGuardias; }
    qreal intervaloDisparos() const { return intervaloDisparo; }
    int corazonesRestantes() const { return corazones; }
    qreal sorpresaRestante() const { return sorpresa; }
    Jugador *obtenerJugador() const { return jugador; }
    const array<GuardiaEntrada *,4> &obtenerGuardias() const { return guardias; }
    const array<Proyectil *,24> &obtenerProyectiles() const { return balas; }
signals:
    void sonidoSolicitado(int evento);
    void informacionCambiada(const QString &texto);
    void controlesLiberados();
    void partidaPerdida();
    void entradaSuperada();
protected:
    void drawBackground(QPainter *,const QRectF &) override;
private:
    Dificultad dificultad = Dificultad::Normal;
    void actualizarPaso(qreal segundos);
    void recibirImpacto();
    void publicarInformacion();
    void actualizarGolpe();
    void disparar(GuardiaEntrada *guardia);
    Jugador *jugador;
    // La escena libera estos objetos; los contenedores solo guardan referencias.
    array<GuardiaEntrada *,4> guardias{};
    array<Proyectil *,24> balas{};
    array<qreal,4> esperaDisparo{};
    QGraphicsRectItem *zonaGolpe; // Hijo del jugador.
    QPixmap paisaje, suelo;
    Estado estado=Disfrazado;
    int corazones=3;
    int cantidadGuardias=2;
    qreal intervaloDisparo=1.5;
    qreal sorpresa=0, inmunidad=0, duracionGolpe=0, recargaGolpe=0, esperaDerrota=0;
    bool golpeUsado=false, derrotaEmitida=false;
    QString ultimoTexto;
};
#endif
