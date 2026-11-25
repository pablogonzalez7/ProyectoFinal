#include "nivel.h"
#include "jugador.h"
#include "hacienda.h"
#include <QApplication>
#include <QGraphicsPixmapItem>
#include <QTimer>
#include <iostream>
#include <stdexcept>
using namespace std;
void exigir(bool condicion,const char *texto) { if (!condicion) throw runtime_error(texto); }
void avanzar(Hacienda &h,qreal segundos)
{
    while(segundos>0.000001) { qreal dt=qMin(segundos,qreal(1.0/120)); h.actualizar(dt); segundos-=dt; }
}
int main(int argc,char **argv)
{
    QApplication app(argc,argv);
    try {
        Nivel nivel;
        Jugador *jugador=nivel.obtenerJugador();
        QGraphicsItem *cama=nivel.obtenerObstaculos().at(14);
        jugador->setPos(200,120); jugador->establecerDireccion({1,0});
        nivel.moverPersonaje(jugador,{200,0});
        exigir(jugador->x()>228,"Sigue chocando contra todo el margen transparente");
        exigir(jugador->x()<260,"Atraveso la cama");
        exigir(!jugador->obtenerImagen()->collidesWithItem(cama),"Quedo dentro del mueble");
        const QPointF limite=jugador->pos(); jugador->moveBy(1,0);
        exigir(jugador->obtenerImagen()->collidesWithItem(cama),"Queda un margen mayor a un pixel");
        jugador->setPos(limite);
        cout<<"OK contacto visible con cama, x="<<limite.x()<<" (antes x=228)"<<endl;
        const QList<QGraphicsItem *> objetos=nivel.obtenerObstaculos();
        const int cantidad=nivel.items().size();
        for(int i=0;i<100;++i) nivel.reiniciarCasa();
        exigir(nivel.obtenerObstaculos()==objetos && nivel.items().size()==cantidad,
               "Reinicio crea mas objetos");
        exigir(jugador->obtenerImagen()->parentItem()==jugador,"Sprite sin propietario");
        cout<<"OK contenedor reutilizado y sprite hijo del personaje"<<endl;
        Hacienda hacienda; Jugador *p=hacienda.obtenerJugador();
        // La primera orilla termina en x=300. El cuerpo tiene 56 px de ancho.
        p->setPos(271,376); hacienda.actualizar(1.0/120);
        exigir(p->estaEnSuelo(),"Cae antes de sacar el centro de los pies");
        p->setPos(273,376); hacienda.actualizar(1.0/120);
        exigir(!p->estaEnSuelo(),"Sigue caminando en el aire hasta sacar toda la cola");
        hacienda.saltar();
        exigir(p->velocidadVerticalActual()>=0,"Permite salto despues de perder apoyo");
        hacienda.actualizar(1.0/120);
        exigir(p->y()>376,"No empieza a caer tras abandonar la tierra");
        cout<<"OK cae al salir el centro de los pies y no salta desde el aire"<<endl;
        hacienda.reiniciar(); hacienda.saltar();
        p->setPos(349,360); p->iniciarVuelo({0,100}); avanzar(hacienda,0.12);
        exigir(!p->estaEnSuelo(),"Aterriza en hipopotamo solo por tocarlo con el borde del sprite");
        hacienda.reiniciar(); hacienda.saltar();
        p->setPos(412,340); p->iniciarVuelo({0,100}); avanzar(hacienda,0.3);
        exigir(p->estaEnSuelo(),"No aterriza con los pies encima del hipopotamo");
        cout<<"OK apoyo real sobre hipopotamo; no aterriza con cola fuera del apoyo"<<endl;
    } catch(const exception &e) { cerr<<e.what()<<endl; return 1; }
}
