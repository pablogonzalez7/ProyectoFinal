#include "introduccion.h"
#include "mainwindow.h"
#include "vistajuego.h"
#include "nivel.h"
#include "jugador.h"
#include "villano.h"
#include "carretera.h"
#include "hacienda.h"
#include "hipopotamo.h"
#include "entradahacienda.h"
#include "guardiaentrada.h"
#include "arenapopeye.h"
#include "popeye.h"
#include "desenlace.h"
#include "proyectil.h"
#include "proyectilcombate.h"
#include "obstaculo.h"
#include "eventossonido.h"
#include <QApplication>
#include <QListWidget>
#include <QTimer>
#include <QDir>
#include <QGraphicsPixmapItem>
#include <QKeyEvent>
#include <iostream>
#include <stdexcept>
using namespace std;
constexpr qreal dt=1.0/120;
void exigir(bool ok,const char *mensaje) { if(!ok) throw runtime_error(mensaje); }
void marcador(MainWindow &v) { QMetaObject::invokeMethod(&v,"moverFigura",Qt::DirectConnection); }
void tecla(QWidget *w,int key) {
 QKeyEvent e(QEvent::KeyPress,key,Qt::NoModifier);QApplication::sendEvent(w,&e);
 QKeyEvent r(QEvent::KeyRelease,key,Qt::NoModifier);QApplication::sendEvent(w,&r);
}
int main(int argc,char **argv)
{
 QApplication app(argc,argv);
 const QString carpeta=argc>1 ? argv[1] : "capturas_modos";QDir().mkpath(carpeta);
 try {
  // Una misma instancia cambia de modo: no conserva vidas ni usos del anterior.
  Nivel casa;
  for (auto modo : {Dificultad::Facil,Dificultad::Normal,Dificultad::Dificil,Dificultad::Facil}) {
   casa.establecerDificultad(modo);casa.reiniciarCasa();auto *j=casa.obtenerJugador();
   int activaciones=0;
   for(int i=0;i<6;++i) {j->alternarCamuflaje();if(j->estaCamuflado())++activaciones;j->actualizarCamuflaje(4.1);}
   exigir(activaciones==(modo==Dificultad::Facil?6:modo==Dificultad::Normal?2:0),"Camuflajes incorrectos segun dificultad");
   casa.reiniciarCasa();j->alternarCamuflaje();exigir(j->estaCamuflado()==(modo!=Dificultad::Dificil),"Reinicio cambia el permiso de camuflaje");
  }
  Hacienda h;
  h.establecerDificultad(Dificultad::Facil);h.reiniciar();
  vector<qreal> fases;
  for(auto *p:h.obtenerPlataformas())if(auto *hip=dynamic_cast<Hipopotamo *>(p))fases.push_back(hip->faseActual());
  exigir(fases==vector<qreal>({0,7.7,6.9,0.4,8.1,7.3,0.8,0}),"Facil no restaura la secuencia original");
  h.reiniciar();int k=0;for(auto *p:h.obtenerPlataformas())if(auto *hip=dynamic_cast<Hipopotamo *>(p))exigir(hip->faseActual()==fases[k++],"Facil cambia la secuencia al reiniciar");
  h.establecerDificultad(Dificultad::Normal);h.reiniciar();bool distinto=false;k=0;
  for(auto *p:h.obtenerPlataformas())if(auto *hip=dynamic_cast<Hipopotamo *>(p))distinto|=hip->faseActual()!=fases[k++];
  exigir(distinto,"Normal sigue usando el patron facil");
  cout<<"OK camuflajes y ciclos por dificultad"<<endl;
  int disparosNormal=0,disparosDificil=0;
  for(auto modo:{Dificultad::Normal,Dificultad::Dificil}) {
   EntradaHacienda e;e.establecerDificultad(modo);e.reiniciar();int disparos=0;
   QObject::connect(&e,&EntradaHacienda::sonidoSolicitado,[&](int sonido){if(sonido==Sonido::Disparo)++disparos;});
   e.transformar();
   for(int i=0;i<120*8;++i){e.obtenerJugador()->setY(0);for(auto *b:e.obtenerProyectiles())b->desactivar();e.actualizar(dt);}
   if(modo==Dificultad::Normal)disparosNormal=disparos;else disparosDificil=disparos;
   exigir(e.intervaloDisparos()==(modo==Dificultad::Dificil?0.8:1.5),"Cadencia incorrecta");
  }
  exigir(disparosDificil>disparosNormal*2,"Los cuatro escoltas no disparan mas rapido");
  cout<<"OK escoltas y cadencia: "<<disparosNormal<<" / "<<disparosDificil<<endl;
  for(int numero=0;numero<3;++numero) {
   const auto modo=static_cast<Dificultad>(numero);
   ArenaPopeye a;a.establecerDificultad(modo);a.reiniciar();
   exigir(a.corazonesRestantes()==5 && a.obtenerPopeye()->resistenciaActual()==(numero==0?5:10),"Vidas iniciales del combate incorrectas");
   for(int obstaculo=1;obstaculo<4;++obstaculo) {
    for(auto tipo:{Directo,Oscilatorio}) {
     auto *b=a.obtenerBalasEnemigas()[0];const auto origen=a.obtenerSuperficies()[obstaculo].center();
     b->activar(origen,origen+QPointF(100,0),true,tipo,1);a.actualizar(0.001);
     exigir(b->estaActivo()==(numero==2),"Colision de bala enemiga no respeta dificultad");b->desactivar();
    }
    auto *b=a.obtenerBalasJugador()[0];const auto origen=a.obtenerSuperficies()[obstaculo].center();
    b->activar(origen,origen+QPointF(100,0),false,Directo,2);a.actualizar(0.001);
    exigir(!b->estaActivo(),"Bala de Perry atraviesa una barrera");
   }
   auto *enemiga=a.obtenerBalasEnemigas()[0];auto centro=a.obtenerJugador()->sceneBoundingRect().center();
   enemiga->activar(centro,centro+QPointF(100,0),true,Directo,3);a.actualizar(0.001);
   exigir(a.corazonesRestantes()==4,"La bala no dana a Perry");
   a.reiniciar();a.activarEscudo();centro=a.obtenerJugador()->sceneBoundingRect().center();
   enemiga->activar(centro,centro+QPointF(100,0),true,Directo,4);a.actualizar(0.001);
   exigir(a.corazonesRestantes()==5 && !enemiga->estaActivo(),"El escudo deja de funcionar");
   // Integracion real: seleccion en menu y transiciones entre escenas.
   MainWindow v;v.show();v.findChild<Introduccion *>()->terminar();v.findChild<QListWidget *>("listaModos")->setCurrentRow(numero);
   QMetaObject::invokeMethod(&v,"elegirModo",Qt::DirectConnection);app.processEvents();
   for(auto *timer:v.findChildren<QTimer *>())timer->stop();
   auto *vista=dynamic_cast<VistaJuego *>(v.findChild<QGraphicsView *>());exigir(vista,"No usa la vista con corazones");
   auto *nivel=v.findChild<Nivel *>();marcador(v);
   exigir(vista->corazonesPerry()==1 && vista->corazonesPopeye()==-1,"Marcador de casa incorrecto");
   v.grab().save(carpeta+QString("/casa_%1.png").arg(numero));
   for(auto *g:nivel->obtenerGuardias())g->setPos(3000,3000);
   for(QPointF lugar:{QPointF(90,680),QPointF(1280,95)}){
    nivel->obtenerJugador()->setPos(lugar);for(int i=0;i<150;++i)nivel->actualizar(dt);
   }
   nivel->obtenerJugador()->setPos(1360,720);nivel->actualizar(dt);app.processEvents();
   auto *carretera=v.findChild<Carretera *>();exigir(carretera && carretera->dificultadActual()==modo,"No propaga dificultad a carretera");
   exigir(carretera->cantidadPerseguidores()==(numero==2?2:1),"Cantidad de carros incorrecta");
   marcador(v);exigir(vista->corazonesPerry()==3 && vista->metrosMostrados()>=0,"HUD carretera incorrecto");
   bool tomo=false;
   const auto objetosCarretera=carretera->items().size();
   for(int i=0;i<120*80 && carretera->obtenerEstado()==Carretera::Conduciendo;++i) {
    for(auto *o:carretera->obtenerObstaculos())o->setX(-3000);
    for(auto *b:carretera->obtenerProyectiles())b->desactivar();
    carretera->actualizar(dt);
    if(!tomo && carretera->distanciaRecorrida()>=450){marcador(v);v.grab().save(carpeta+QString("/carretera_%1.png").arg(numero));tomo=true;}
   }
   exigir(carretera->items().size()==objetosCarretera,"Recrea objetos de decoracion al avanzar");
   marcador(v);exigir(vista->metrosMostrados()==1200,"Contador no llega a 1200");
   carretera->activarDimensionador();app.processEvents();
   auto *rio=v.findChild<Hacienda *>();exigir(rio && rio->dificultadActual()==modo,"No propaga dificultad al rio");
   marcador(v);exigir(vista->corazonesPerry()==3 && vista->metrosMostrados()==-1,"HUD conserva metros fuera de carretera");
   v.grab().save(carpeta+QString("/rio_%1.png").arg(numero));
   rio->obtenerJugador()->setPos(345,450);rio->obtenerJugador()->establecerEnSuelo(false);rio->actualizar(dt);marcador(v);
   exigir(vista->corazonesPerry()==2,"No elimina corazon al caer al agua");
   rio->actualizar(1.3);auto *j=rio->obtenerJugador();j->setPos(4220,320);j->establecerEnSuelo(false);
   for(int i=0;i<120;++i)rio->actualizar(dt);
   app.processEvents();
   auto *entrada=v.findChild<EntradaHacienda *>();exigir(entrada && entrada->dificultadActual()==modo,"No propaga dificultad a escoltas");
   const int cantidad=numero==0?1:numero==2?4:2;exigir(entrada->numeroGuardias()==cantidad,"Numero de escoltas incorrecto");
   int visibles=0;for(auto *g:entrada->obtenerGuardias())visibles+=g->isVisible();exigir(visibles==cantidad,"Guardias ocultos o extra");
   marcador(v);exigir(vista->corazonesPerry()==3,"HUD escoltas incorrecto");
   v.grab().save(carpeta+QString("/escoltas_%1.png").arg(numero));
   entrada->transformar();for(auto *g:entrada->obtenerGuardias())g->cambiarEstado(GuardiaEntrada::Inconsciente);
   j=entrada->obtenerJugador();j->establecerEnSuelo(false);entrada->golpear();
   j->setPos(850,376);j->aterrizar(420);entrada->golpear();app.processEvents();
   auto *arena=v.findChild<ArenaPopeye *>();exigir(arena && arena->dificultadActual()==modo,"No propaga dificultad a Popeye");
   marcador(v);exigir(vista->corazonesPerry()==5 && vista->corazonesPopeye()==(numero==0?5:10),"HUD jefe incorrecto");
   v.grab().save(carpeta+QString("/popeye_%1.png").arg(numero));
   arena->obtenerPopeye()->recibirDano(2);marcador(v);
   exigir(vista->corazonesPopeye()==(numero==0?3:8),"No quita corazones de Popeye");
   auto *b=arena->obtenerBalasEnemigas()[0];centro=arena->obtenerJugador()->sceneBoundingRect().center();
   b->activar(centro,centro+QPointF(100,0),true,Directo,50);arena->actualizar(0.001);marcador(v);
   exigir(vista->corazonesPerry()==4,"No quita corazon de Perry en combate");
   tecla(vista,Qt::Key_R);marcador(v);
   exigir(arena->dificultadActual()==modo && vista->corazonesPerry()==5 && vista->corazonesPopeye()==(numero==0?5:10),"R pierde dificultad o corazones");
   arena->obtenerPopeye()->recibirDano(10);arena->actualizar(dt);tecla(vista,Qt::Key_Return);app.processEvents();
   auto *fin=v.findChild<Desenlace *>();exigir(fin,"No llega al desenlace");marcador(v);
   exigir(vista->corazonesPerry()==fin->corazonesRestantes() && vista->corazonesPopeye()==-1,"No conserva corazones en desenlace");
   v.grab().save(carpeta+QString("/desenlace_%1.png").arg(numero));
   cout<<"OK modo "<<numero<<": colisiones, transiciones, corazones, escenas y reinicio"<<endl;
  }
 } catch(const exception &e){cerr<<"ERROR: "<<e.what()<<endl;return 1;}
 return 0;
}
