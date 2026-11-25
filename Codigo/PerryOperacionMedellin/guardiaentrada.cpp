#include "guardiaentrada.h"
#include "recursossprites.h"
#include <QGraphicsRectItem>
#include <QPainter>
#include <cmath>
using namespace std;

GuardiaEntrada::GuardiaEntrada(qreal posicionInicial)
    : Personaje(42), cabeza(new QGraphicsRectItem(2,6,28,16,this)), inicio(posicionInicial)
{
    cabeza->setPen(Qt::NoPen);
    cabeza->setBrush(Qt::NoBrush);
    const QPixmap hoja(":/imagenes/policias.png");
    const int ancho = hoja.width()/4, alto = hoja.height()/2;
    for (int lado=0; lado<2; ++lado) {
        for (int paso=0; paso<2; ++paso) {
            const QRect recorte((3-lado)*ancho+ancho*22/100,
                                paso*alto+alto*4/100,ancho*56/100,alto*92/100);
            fotogramas[lado][paso] = RecursosSprites::cargar(
                ":/imagenes/policias.png",QSize(32,64),recorte);
        }
    }
    setZValue(4);
    reiniciar();
}
QRectF GuardiaEntrada::boundingRect() const { return QRectF(0,0,32,64); }
void GuardiaEntrada::reiniciar()
{
    estado=Acercandose;
    izquierda=true;
    caminando=false;
    tiempoAnimacion=0;
    setRotation(0);
    setOpacity(1);
    setPos(inicio,356);
    mostrarSprite();
}
void GuardiaEntrada::cambiarEstado(Estado nuevoEstado)
{
    estado=nuevoEstado;
    caminando=false;
    if (estado==Inconsciente) {
        // Reutiliza el mismo sprite, acostado sobre el piso.
        setTransformOriginPoint(16,64);
        setRotation(-90);
        setY(340);
        setOpacity(0.65);
    }
    mostrarSprite();
    update();
}
void GuardiaEntrada::seguir(qreal posicionJugador,qreal segundos)
{
    caminando=false;
    if (estado==Sorprendido || estado==Inconsciente) return;
    const qreal diferencia=posicionJugador-(x()+16);
    izquierda=diferencia<0;
    const qreal separacion=estado==Acercandose ? 100 : 135;
    if (abs(diferencia)>separacion) {
        const qreal avance=qMin(velocidad*segundos,abs(diferencia)-separacion);
        setX(qBound(qreal(100),x()+(izquierda ? -avance : avance),qreal(840)));
        caminando=true;
    }
}
void GuardiaEntrada::actualizar(qreal segundos)
{
    if (caminando) tiempoAnimacion=fmod(tiempoAnimacion+segundos,0.36);
    else tiempoAnimacion=0;
    mostrarSprite();
}
void GuardiaEntrada::mostrarSprite()
{
    mostrarFotograma(fotogramas[izquierda ? 0 : 1][tiempoAnimacion>=0.18 ? 1 : 0]);
}
void GuardiaEntrada::paint(QPainter *p,const QStyleOptionGraphicsItem *o,QWidget *w)
{
    Personaje::paint(p,o,w);
    if (estado==Sorprendido) {
        p->setPen(QColor("#ffe18c"));
        p->setFont(QFont("Arial",14,QFont::Bold));
        p->drawText(QRectF(0,0,32,20),Qt::AlignCenter,"!");
    }
}
