#ifndef OBSTACULO_H
#define OBSTACULO_H
#include <QGraphicsItem>
using namespace std;
class Obstaculo : public QGraphicsItem {
public:
    enum Tipo { Barrera, Lodo };
    explicit Obstaculo(QGraphicsItem *padre=nullptr);
    QRectF boundingRect() const override;
    void paint(QPainter *,const QStyleOptionGraphicsItem *,QWidget *) override;
    void preparar(Tipo nuevoTipo,qreal centroX,qreal altura);
    Tipo obtenerTipo() const { return tipo; }
    bool fueGolpeado() const { return golpeado; }
    void marcarGolpe();
private:
    Tipo tipo=Barrera;
    bool golpeado=false;
};
#endif
