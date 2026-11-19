#ifndef NIVEL_H
#define NIVEL_H

#include <QGraphicsScene>
#include <QList>
#include <QPointF>
#include <QRectF>

class Nivel : public QGraphicsScene
{
public:
    explicit Nivel(QObject *padre = nullptr);

    QPointF limitarDesplazamiento(
        const QRectF &cuerpo,
        const QPointF &desplazamiento
        ) const;

private:
    const QList<QRectF> paredes;
};

#endif // NIVEL_H
