#include "introduccion.h"
#include "entradahacienda.h"
#include "guardiaentrada.h"
#include "jugador.h"
#include "proyectil.h"
#include "mainwindow.h"
#include "nivel.h"
#include "villano.h"
#include "carretera.h"
#include "obstaculo.h"
#include "hacienda.h"
#include <QApplication>
#include <QGraphicsRectItem>
#include <QGraphicsView>
#include <QKeyEvent>
#include <QTimer>
#include <QPainter>
#include <QImage>
#include <QDir>
#include <iostream>
#include <stdexcept>
#include <cmath>
using namespace std;
constexpr qreal paso=1.0/120;
void exigir(bool condicion,const char *mensaje) { if (!condicion) throw runtime_error(mensaje); }
void avanzar(EntradaHacienda &e,qreal segundos) {
    for (int i=0;i<int(round(segundos/paso));++i) e.actualizar(paso);
}
void capturar(EntradaHacienda &e,const QString &ruta) {
    QImage imagen(960,540,QImage::Format_ARGB32_Premultiplied); imagen.fill(Qt::transparent);
    QPainter p(&imagen); e.render(&p); p.end(); imagen.save(ruta);
}
int main(int argc,char **argv) {
 QApplication app(argc,argv);
 try {
    const QString carpeta=argc>1 ? QString::fromLocal8Bit(argv[1]) : "capturas_entrada";
    QDir().mkpath(carpeta);
    EntradaHacienda e;
    auto *j=e.obtenerJugador();
    const auto cantidad=e.items().size();
    const auto guardias=e.obtenerGuardias();
    const auto balas=e.obtenerProyectiles();
    capturar(e,carpeta+"/entrada_disfraz.png");
    avanzar(e,8);
    exigir(e.obtenerEstado()==EntradaHacienda::Disfrazado && e.corazonesRestantes()==3,"Atacan antes de transformarse");
    for (auto *b:balas) exigir(!b->estaActivo(),"Disparan al disfrazado");
    e.transformar();
    const qreal x=e.obtenerGuardias()[0]->x();
    avanzar(e,2);
    e.transformar();
    exigir(abs(e.sorpresaRestante()-1)<0.001,"C renueva la sorpresa");
    exigir(e.obtenerGuardias()[0]->x()==x,"Guardia se mueve sorprendido");
    avanzar(e,1.01);
    exigir(e.obtenerEstado()==EntradaHacienda::Combate,"Sorpresa no termina a los 3 segundos");
    cout<<"OK disfraz, sorpresa unica de tres segundos y guardias detenidos"<<endl;
    avanzar(e,1.6);
    Proyectil *disparo=nullptr;
    for (auto *b:balas) if (b->estaActivo()) { disparo=b; break; }
    exigir(disparo,"No dispara despues de la sorpresa");
    const QPointF posicion=disparo->pos();
    avanzar(e,0.02);
    exigir(disparo->y()==posicion.y() && disparo->x()!=posicion.x(),"Disparo no horizontal");
    // Comprobaciones aisladas de zonas: el cuerpo y el golpe en suelo no cuentan.
    e.reiniciar();e.transformar();
    j->setPos(490,376);j->aterrizar(420);e.golpear();e.actualizar(paso);
    exigir(guardias[0]->estaActivo(),"Golpe desde suelo vence al escolta");
    j->establecerEnSuelo(false); j->setPos(490,367);e.golpear();e.actualizar(paso);
    exigir(guardias[0]->estaActivo(),"Golpe al torso vence al escolta");
    cout<<"OK disparos horizontales, cuerpo y suelo no incapacitan"<<endl;
    // Recorrido real: solo movimiento, salto y golpe, sin cambiar posiciones.
    e.reiniciar();
    e.establecerDireccion(1);
    while (j->x()<350) e.actualizar(paso);
    e.establecerDireccion(0);e.transformar();
    capturar(e,carpeta+"/entrada_sorpresa.png");
    bool capturoPatada=false;
    for (int i=0;i<120*20 && e.obtenerEstado()!=EntradaHacienda::Despejada;++i) {
        GuardiaEntrada *objetivo=guardias[0]->estaActivo() ? guardias[0] : guardias[1];
        const qreal diferencia=objetivo->x()-j->x();
        e.establecerDireccion(diferencia>56 ? 1 : diferencia<42 ? -1 : 0);
        if (j->estaEnSuelo() && diferencia<130) e.saltar();
        if (!j->estaEnSuelo() && j->y()+44>objetivo->y()+6 && j->y()+26<objetivo->y()+22) {
            e.golpear();
            if (!capturoPatada) {capturar(e,carpeta+"/entrada_patada.png");capturoPatada=true;}
        }
        e.actualizar(paso);
        exigir(e.obtenerEstado()!=EntradaHacienda::Derrota,"Piloto pierde contra escoltas");
    }
    exigir(e.obtenerEstado()==EntradaHacienda::Despejada,"Piloto no vence a ambos escoltas");
    exigir(!guardias[0]->estaActivo() && !guardias[1]->estaActivo(),"Faltan escoltas");
    int salidas=0;QObject::connect(&e,&EntradaHacienda::entradaSuperada,[&]{++salidas;});
    e.golpear();exigir(salidas==0,"Abre puerta desde lejos");
    e.establecerDireccion(1);
    while (j->x()<835 || !j->estaEnSuelo()) e.actualizar(paso);
    e.establecerDireccion(0);e.golpear();e.golpear();
    exigir(e.obtenerEstado()==EntradaHacienda::Completada && salidas==1,"Entrada no se completa una sola vez");
    capturar(e,carpeta+"/entrada_despejada.png");
    cout<<"OK recorrido real: vence a ambos, abre puerta; corazones="<<e.corazonesRestantes()<<endl;
    // Una rafaga simultanea consume un corazon, nunca varios.
    e.reiniciar();e.transformar();avanzar(e,3.01);
    auto impactar=[&] {
        for (int i=0;i<2;++i) balas[i]->activar(j->sceneBoundingRect().center(),j->sceneBoundingRect().center()+QPointF(100,0));
        e.actualizar(paso);
    };
    impactar();exigir(e.corazonesRestantes()==2,"Impactos simultaneos gastan varios corazones");
    int derrotas=0;QObject::connect(&e,&EntradaHacienda::partidaPerdida,[&]{++derrotas;});
    for (int golpe=0;golpe<2;++golpe) {
        for (int i=0;i<120;++i) {for (auto *b:balas)b->desactivar();e.actualizar(paso);}
        impactar();
    }
    exigir(e.obtenerEstado()==EntradaHacienda::Derrota && e.corazonesRestantes()==0,"No pierde al tercer impacto");
    e.actualizar(1.3);e.actualizar(2);
    exigir(derrotas==1,"Derrota repetida o ausente");
    for (int i=0;i<100;++i)e.reiniciar();
    exigir(e.items().size()==cantidad && e.obtenerGuardias()==guardias && e.obtenerProyectiles()==balas,"Reinicio recrea objetos");
    cout<<"OK inmunidad, derrota unica y cien reinicios con los mismos objetos"<<endl;
    // Integracion de escenas y teclado de la ventana real.
    MainWindow ventana;ventana.show(); ventana.findChild<Introduccion *>()->terminar(); QMetaObject::invokeMethod(&ventana, "elegirModo", Qt::DirectConnection);app.processEvents();ventana.findChild<QTimer *>()->stop();
    auto *nivel=ventana.findChild<Nivel *>();auto *vista=ventana.findChild<QGraphicsView *>();
    for (auto *g:nivel->obtenerGuardias())g->setPos(3000,3000);
    for (QPointF lugar:{QPointF(90,680),QPointF(1280,95)}) {
        nivel->obtenerJugador()->setPos(lugar);for(int i=0;i<150;++i)nivel->actualizar(paso);
    }
    nivel->obtenerJugador()->setPos(1360,720);nivel->actualizar(paso);app.processEvents();
    auto *carretera=ventana.findChild<Carretera *>();exigir(carretera,"No entra a carretera");
    for(int i=0;i<120*80 && carretera->obtenerEstado()==Carretera::Conduciendo;++i) {
        for(auto *o:carretera->obtenerObstaculos())o->setX(-3000);
        for(auto *b:carretera->obtenerProyectiles())b->desactivar();
        carretera->actualizar(paso);
    }
    carretera->activarDimensionador();app.processEvents();
    auto *hacienda=ventana.findChild<Hacienda *>();exigir(hacienda,"No entra a hacienda");
    auto *pj=hacienda->obtenerJugador();pj->setPos(4220,320);pj->establecerEnSuelo(false);
    for(int i=0;i<120;++i)hacienda->actualizar(paso);
    app.processEvents();
    auto *entrada=ventana.findChild<EntradaHacienda *>();
    exigir(entrada && vista->scene()==entrada,"Lianas no conectan con escoltas");
    QKeyEvent repetida(QEvent::KeyPress,Qt::Key_C,Qt::NoModifier,QString(),true);
    QApplication::sendEvent(vista,&repetida);exigir(entrada->obtenerEstado()==EntradaHacienda::Disfrazado,"Autorrepeticion transforma");
    QKeyEvent c(QEvent::KeyPress,Qt::Key_C,Qt::NoModifier);QApplication::sendEvent(vista,&c);
    exigir(entrada->obtenerEstado()==EntradaHacienda::Sorpresa,"C no transforma en la ventana");
    QKeyEvent salto(QEvent::KeyPress,Qt::Key_Space,Qt::NoModifier);QApplication::sendEvent(vista,&salto);
    exigir(entrada->obtenerJugador()->velocidadVerticalActual()<0,"Espacio no salta en entrada");
    QKeyEvent reinicio(QEvent::KeyPress,Qt::Key_R,Qt::NoModifier);QApplication::sendEvent(vista,&reinicio);
    exigir(entrada->obtenerEstado()==EntradaHacienda::Disfrazado,"R no reinicia encuentro");
    entrada->transformar();avanzar(*entrada,3.01);
    for(int k=0;k<3;++k) {
        auto *b=entrada->obtenerProyectiles()[0];auto centro=entrada->obtenerJugador()->sceneBoundingRect().center();
        b->activar(centro,centro+QPointF(100,0));entrada->actualizar(paso);
        if(k<2)for(int i=0;i<120;++i) {for(auto *p:entrada->obtenerProyectiles())p->desactivar();entrada->actualizar(paso);}
    }
    entrada->actualizar(1.3);app.processEvents();
    exigir(vista->scene()==hacienda && hacienda->obtenerJugador()->x()==100 && hacienda->vidasRestantes()==3,"Derrota no vuelve al inicio de hacienda");
    cout<<"OK transicion real, C, espacio, R y retorno al inicio de la hacienda"<<endl;
 } catch(const exception &e) {cerr<<"ERROR: "<<e.what()<<endl;return 1;}
 return 0;
}
