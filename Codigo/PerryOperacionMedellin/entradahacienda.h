#ifndef ENTRADAHACIENDA_H
#define ENTRADAHACIENDA_H
#include <QGraphicsScene>
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
    QString informacionActual() const;
    Estado obtenerEstado() const { return estado; }
    int corazonesRestantes() const { return corazones; }
    qreal sorpresaRestante() const { return sorpresa; }
    Jugador *obtenerJugador() const { return jugador; }
    const array<GuardiaEntrada *,2> &obtenerGuardias() const { return guardias; }
    const array<Proyectil *,12> &obtenerProyectiles() const { return balas; }
signals:
    void informacionCambiada(const QString &texto);
    void controlesLiberados();
    void partidaPerdida();
    void entradaSuperada();
protected:
    void drawBackground(QPainter *,const QRectF &) override;
private:
    void actualizarPaso(qreal segundos);
    void recibirImpacto();
    void publicarInformacion();
    void actualizarGolpe();
    void disparar(GuardiaEntrada *guardia);
    Jugador *jugador;
    // La escena libera estos objetos; los contenedores solo guardan referencias.
    array<GuardiaEntrada *,2> guardias{};
    array<Proyectil *,12> balas{};
    array<qreal,2> esperaDisparo{};
    QGraphicsRectItem *zonaGolpe; // Hijo del jugador.
    QPixmap paisaje, suelo;
    Estado estado=Disfrazado;
    int corazones=3;
    qreal sorpresa=0, inmunidad=0, duracionGolpe=0, recargaGolpe=0, esperaDerrota=0;
    bool golpeUsado=false, derrotaEmitida=false;
    QString ultimoTexto;
};
#endif
