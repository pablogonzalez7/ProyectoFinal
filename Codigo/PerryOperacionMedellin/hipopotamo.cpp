#include "hipopotamo.h"
#include "configuracionhacienda.h"
#include <QPainter>
#include <QRandomGenerator>
#include <QPixmapCache>
#include <stdexcept>
#include <cmath>
using namespace std;
using namespace ConfiguracionHacienda;
Hipopotamo::Hipopotamo(qreal x, qreal y, qreal desfase)
    : Plataforma(x, y, 140, 100), fase(desfase) { cargarSprites(); }
bool Hipopotamo::permiteApoyo() const { return fase < inicioInmersion; }
bool Hipopotamo::estaAvisando() const { return fase >= inicioAviso && fase < inicioInmersion; }
qreal Hipopotamo::faseActual() const { return fase; }
void Hipopotamo::reiniciar()
{
    // Cada hipopotamo empieza en un momento distinto del mismo ciclo.
    // El aviso sigue durando un segundo y no cambian dibujo ni colisiones.
    fase = QRandomGenerator::global()->generateDouble() * periodoHipopotamo;
    update();
}
void Hipopotamo::actualizar(qreal dt)
{
    fase = fmod(fase + dt, periodoHipopotamo);
    update();
}
QRectF Hipopotamo::boundingRect() const { return QRectF(0, -26, 140, 126); }

void Hipopotamo::cargarSprites()
{
    for (int i = 0; i < 4; ++i) {
        const QString clave = QString("hipopotamo_sprite_v1_%1").arg(i);
        if (QPixmapCache::find(clave, &sprites[i])) continue;
        const QPixmap hoja(":/imagenes/hipopotamos.png");
        if (hoja.isNull() || hoja.width() < 2 || hoja.width() != hoja.height() || hoja.width() % 2 != 0)
            throw runtime_error("No se pudo cargar la hoja cuadrada de hipopotamos con cuatro celdas.");
        // Cuatro celdas. Margenes transparentes recortados una sola vez.
        const int celda = hoja.width() / 2;
        sprites[i] = hoja.copy(QRect((i % 2) * celda, (i / 2) * celda + celda * 200 / 640,
                                     celda, celda * 390 / 640))
                         .scaled(140, 85, Qt::IgnoreAspectRatio, Qt::FastTransformation);
        QPixmapCache::insert(clave, sprites[i]);
    }
}

void Hipopotamo::paint(QPainter *p, const QStyleOptionGraphicsItem *, QWidget *)
{
    qreal hundimiento = 0;
    if (fase >= inicioInmersion && fase < 6) hundimiento = (fase - inicioInmersion) * 2;
    else if (fase >= 6 && fase < inicioSalida) hundimiento = 1;
    else if (fase >= inicioSalida) hundimiento = (periodoHipopotamo - fase) * 2;

    if (hundimiento >= 0.999) {
        p->drawPixmap(0, -26, sprites[3]);
        return;
    }
    const int indice = estaAvisando() ? 2 : (fmod(fase, 2.0) > 1.7 ? 1 : 0);
    p->save();
    p->setClipRect(QRectF(0, -26, 140, 85));
    p->setOpacity(1.0 - hundimiento);
    p->drawPixmap(QPointF(0, -26 + hundimiento * 35), sprites[indice]);
    p->restore();
    if (hundimiento > 0) {
        p->save();
        p->setOpacity(hundimiento);
        p->drawPixmap(0, -26, sprites[3]);
        p->restore();
    }
    // Marca discreta alineada con la superficie fisica de aterrizaje.
    if (permiteApoyo()) {
        p->setPen(QPen(QColor(estaAvisando() ? "#ffe18c" : "#c7eed9"), 2));
        p->drawLine(QPointF(12, 0), QPointF(75, 0));
    }
    if (estaAvisando()) {
        p->setPen(QColor("#ffe18c"));
        p->drawText(QRectF(108, -26, 28, 20), Qt::AlignCenter, "!");
    }
}
