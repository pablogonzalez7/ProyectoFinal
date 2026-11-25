#ifndef PERSONAJE_H
#define PERSONAJE_H

#include <QGraphicsObject>
#include <QGraphicsPixmapItem>

using namespace std;

class Personaje : public QGraphicsObject
{
    Q_OBJECT

public:
    explicit Personaje(qreal velocidadInicial,
                       QGraphicsItem *padre = nullptr);

    ~Personaje() override = default;

    QRectF boundingRect() const override;

    void paint(QPainter *pintor,
               const QStyleOptionGraphicsItem *opciones,
               QWidget *widget) override;

    QGraphicsPixmapItem *obtenerImagen() const;

    virtual void actualizar(qreal segundosTranscurridos) = 0;

protected:
    void mostrarFotograma(const QPixmap &fotograma);
    qreal velocidad;

private:
    QGraphicsPixmapItem *imagen;
};

#endif // PERSONAJE_H
