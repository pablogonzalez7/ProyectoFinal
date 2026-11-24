#ifndef VEHICULO_H
#define VEHICULO_H
#include <QGraphicsItem>
#include <QPixmap>
using namespace std;
class Vehiculo : public QGraphicsItem {
public:
    explicit Vehiculo(QGraphicsItem *padre=nullptr);
    QRectF boundingRect() const override;
    void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *) override;
    void establecerDireccion(int direccion);
    void actualizar(qreal segundos, const QRectF &limites);
    void restablecer();
    qreal rapidezAvance() const;
    qreal velocidadMetrosSegundo() const { return velocidad; }
    void actualizarVelocidad(qreal segundos,bool enLodo);
    void recibirImpacto();
    void establecerEnemigo(bool valor) { enemigo=valor; update(); }
private:
    QPixmap spriteJugador, spriteEnemigo;
    int direccionHorizontal=0;
    qreal velocidad=28.0;
    bool enemigo=false;
};
#endif
