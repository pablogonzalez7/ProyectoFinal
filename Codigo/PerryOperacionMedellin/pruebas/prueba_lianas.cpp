#include "hacienda.h"
#include "liana.h"
#include "jugador.h"
#include "plataforma.h"
#include "hipopotamo.h"
#include "configuracionhacienda.h"
#include "mainwindow.h"
#include "nivel.h"
#include "villano.h"
#include "carretera.h"
#include "vehiculo.h"
#include "obstaculo.h"
#include "proyectil.h"
#include <QApplication>
#include <QGraphicsView>
#include <QTimer>
#include <QKeyEvent>
#include <QImage>
#include <QPainter>
#include <QDir>
#include <iostream>
#include <cmath>
#include <stdexcept>
using namespace std;
constexpr qreal paso=1.0/120;
void exigir(bool ok,const char *s) { if(!ok) throw runtime_error(s); }
void avanzar(Hacienda &h,qreal t) { for(int i=0;i<int(round(t/paso));++i) h.actualizar(paso); }
void capturar(Hacienda &h,const char *nombre,qreal centro=480) {
 QImage im(960,540,QImage::Format_ARGB32_Premultiplied); im.fill(Qt::transparent);
 QPainter p(&im); h.render(&p,QRectF(0,0,960,540),QRectF(centro-480,0,960,540)); p.end(); im.save(nombre);
}
int main(int argc,char **argv) {
 QApplication app(argc,argv);
 QDir().mkpath("capturas_lianas");
 try {
  Liana pendulo(QPointF(50,70),260,-0.85);
  const auto energia=[&] { return 0.5*pow(260*pendulo.velocidadAngularActual(),2)+1400*260*(1-cos(pendulo.anguloActual())); };
  const qreal inicial=energia(); qreal errorMaximo=0;
  for(int i=0;i<120*60;++i) {
    pendulo.actualizar(paso);
    errorMaximo=qMax(errorMaximo,abs(energia()-inicial)/inicial);
    QPointF radio=pendulo.puntoAgarre()-pendulo.pos();
    exigir(abs(QPointF::dotProduct(radio,radio)-260*260)<0.00001,"Longitud variable");
    exigir(abs(QPointF::dotProduct(radio,pendulo.velocidadTangencial()))<0.00001,"Velocidad no tangente");
  }
  exigir(errorMaximo<0.001,"Energia del pendulo deriva");
  cout<<"OK pendulo 60 segundos, error relativo energia="<<errorMaximo<<endl;
  Hacienda h; auto *j=h.obtenerJugador(); const auto cuenta=h.items().size();
  exigir(j->estaEnSuelo(),"No inicia apoyado");
  h.saltar(); avanzar(h,0.2);
  const qreal esperado=376-540*0.2+700*0.2*0.2;
  exigir(abs(j->y()-esperado)<0.0001,"Parabola incorrecta");
  const auto vy=j->velocidadVerticalActual(); h.saltar();
  exigir(j->velocidadVerticalActual()==vy,"Permite doble salto");
  avanzar(h,1); exigir(j->estaEnSuelo() && abs(j->y()-376)<0.001,"No aterriza en orilla");
  cout<<"OK parabola exacta, rechazo de doble salto y aterrizaje"<<endl;
  Hipopotamo hip(0,0,0); hip.actualizar(4.6);
  exigir(hip.estaAvisando() && hip.permiteApoyo(),"Aviso incorrecto");
  hip.actualizar(1.0); exigir(!hip.permiteApoyo(),"Apoya sumergido");
  hip.actualizar(3); exigir(hip.permiteApoyo(),"No emerge");
  hip.reiniciar(); exigir(hip.faseActual()>=0 && hip.faseActual()<8.5,"Fase fuera del ciclo");
  cout<<"OK aviso, inmersion y ciclo de hipopotamo"<<endl;
  h.reiniciar();
  auto *inicialHip=dynamic_cast<Hipopotamo *>(h.obtenerPlataformas()[1]);
  inicialHip->actualizar(8.5-inicialHip->faseActual());
  j->setPos(440,320); j->establecerEnSuelo(false); avanzar(h,0.5);
  exigir(j->estaEnSuelo() && abs(j->sceneBoundingRect().bottom()-h.obtenerPlataformas()[1]->superficie().top())<0.001,"No aterriza en hipopotamo");
  auto *primero=dynamic_cast<Hipopotamo *>(h.obtenerPlataformas()[1]);
  primero->actualizar(5.6-primero->faseActual()); h.actualizar(paso);
  exigir(!j->estaEnSuelo(),"Flota cuando se hunde hipopotamo");
  avanzar(h,0.4); exigir(h.obtenerEstado()==Hacienda::Recuperando,"No cae al agua tras hundirse");
  h.reiniciar(); h.saltar(); j->setPos(440,416); avanzar(h,0.1);
  exigir(!j->estaEnSuelo() && j->velocidadVerticalActual()<0,"Aterriza subiendo desde abajo");
  h.reiniciar();
  cout<<"OK perdida de apoyo al sumergirse y colision solo descendente"<<endl;
  for(int i=0;i<3;++i) {
    j->setPos(345,450); j->establecerEnSuelo(false); h.actualizar(paso);
    exigir(h.vidasRestantes()==2-i,"Vidas incorrectas");
    exigir(h.obtenerEstado()==Hacienda::Recuperando,"No pausa tras caer");
    h.actualizar(1.3);
  }
  exigir(h.vidasRestantes()==3 && j->x()==100 && j->estaEnSuelo(),"No reinicia hacienda al perder tres vidas");
  cout<<"OK tres vidas y reinicio de hacienda"<<endl;
  // Piloto usa los mismos controles: espera en orillas, salta y corrige A/D.
  // Esta regresion de saltos y lianas usa fases conocidas para ser reproducible.
  // Las fases aleatorias se verifican aparte en prueba_azar_camuflaje.cpp.
  h.reiniciar();
  const qreal fasesPrueba[] = {0, 7.7, 6.9, 0.4, 8.1, 7.3, 0.8, 0};
  int indiceHip = 0;
  for (auto *p : h.obtenerPlataformas()) {
    if (auto *hip = dynamic_cast<Hipopotamo *>(p)) {
      hip->actualizar(fmod(fasesPrueba[indiceHip++] - hip->faseActual() + 8.5, 8.5));
    }
  }
  int destino=1; int saltos=0; bool inicioVuelo=false; int ultimaVida=3;
  for(int i=0;i<120*180 && h.obtenerEstado()!=Hacienda::Completado;++i) {
    if(h.obtenerEstado()!=Hacienda::EnJuego) { h.actualizar(paso); continue; }
    if(h.vidasRestantes()!=ultimaVida) {
        cout<<"Piloto perdio vida x="<<j->x()<<" destino="<<destino<<endl;
        ultimaVida=h.vidasRestantes(); inicioVuelo=false;
        destino=1;
        for(size_t k=0;k<h.obtenerPlataformas().size();++k)
           if(j->x()>=h.obtenerPlataformas()[k]->x()) destino=int(k)+1;
    }
    auto *actual=h.obtenerPlataformas()[destino-1];
    auto *sig=h.obtenerPlataformas()[destino];
    if(j->estaEnSuelo() && inicioVuelo) {
      ++destino; inicioVuelo=false;
      if(destino>=12) {
       h.establecerDireccion(1); avanzar(h,1); break;
      }
      actual=h.obtenerPlataformas()[destino-1]; sig=h.obtenerPlataformas()[destino];
    }
    if(j->estaEnSuelo()) {
      const qreal despegue=actual->superficie().right()-j->boundingRect().width()-5;
      if(j->x()<despegue-2) h.establecerDireccion(1);
      else {
        h.establecerDireccion(0);
        const qreal duracion=(540+sqrt(540*540+2800*(sig->y()-actual->y())))/1400;
        auto *siguienteHip=dynamic_cast<Hipopotamo *>(sig);
        const bool seguro=!siguienteHip || fmod(siguienteHip->faseActual()+duracion,8.5)<4.4;
        if(seguro) { h.saltar(); inicioVuelo=true; ++saltos; }
      }
    } else {
      qreal objetivo=sig->x()+18;
      h.establecerDireccion(abs(j->x()-objetivo)<2 ? 0 : j->x()<objetivo ? 1 : -1);
    }
    h.actualizar(paso);
    if(i==35) capturar(h,"capturas_lianas/sprites_inicio.png");
  }
  cout<<"Piloto: estado="<<h.obtenerEstado()<<" vidas="<<h.vidasRestantes()<<" x="<<j->x()<<" saltos="<<saltos<<endl;
  exigir(j->x()>2840 && j->estaEnSuelo(),"No logra cruzar el rio");
  exigir(h.vidasRestantes()==3,"Piloto pierde vidas en el rio");
  for(int indice=0;indice<2;++indice) {
    auto *liana=h.obtenerLianas()[indice];
    auto *orilla=h.obtenerPlataformas()[11+indice];
    auto *destinoLiana=h.obtenerPlataformas()[12+indice];
    const qreal borde=orilla->superficie().right()-j->boundingRect().width()-5;
    h.establecerDireccion(1);
    for(int i=0;i<120*5 && j->x()<borde-2;++i) h.actualizar(paso);
    h.establecerDireccion(0); h.establecerAgarre(true);
    bool solto=false; int intentos=0;
    for(int i=0;i<120*30;++i) {
      if(j->estaEnSuelo() && j->x()<destinoLiana->x()
         && liana->anguloActual()<-0.65 && liana->velocidadAngularActual()<0) {
        h.saltar(); ++intentos;
      }
      if(h.estaEnLiana() && liana->anguloActual()>0.42 && liana->velocidadAngularActual()>0) {
        capturar(h,"capturas_lianas/lianas_balanceo.png",3590);
        QPointF velocidad=liana->velocidadTangencial(); QPointF origen=j->pos();
        h.saltar(); solto=true;
        exigir(!h.estaEnLiana(),"No suelta la liana");
        exigir((j->velocidadVueloActual()-velocidad).manhattanLength()<0.00001,"No conserva velocidad tangente");
        h.establecerDireccion(-1); // A/D no debe alterar este vuelo.
        avanzar(h,0.05);
        exigir(abs(j->x()-origen.x()-velocidad.x()*0.05)<0.00001,"A/D cambia vuelo inercial");
        exigir(abs(j->y()-origen.y()-velocidad.y()*0.05-700*0.05*0.05)<0.00001,"Vuelo no parabolico");
        exigir(!h.estaEnLiana(),"Se reagarró inmediatamente");
        h.establecerDireccion(0);
      }
      h.actualizar(paso);
      if(solto && j->estaEnSuelo()) break;
      exigir(h.vidasRestantes()==3,"Piloto pierde vida en lianas");
    }
    cout<<"Liana "<<indice+1<<": x="<<j->x()<<" intentos="<<intentos<<endl;
    exigir(solto && j->estaEnSuelo() && j->x()+56>destinoLiana->x(),"No cruza liana");
    h.establecerAgarre(false);
  }
  h.establecerDireccion(1); avanzar(h,1);
  exigir(h.obtenerEstado()==Hacienda::Completado,"No llega a entrada vigilada");
  cout<<"OK rio y dos lianas con controles reales, tres vidas intactas"<<endl;
  // Perder una vida en la segunda liana conserva el punto seguro intermedio.
  Hacienda chequeo; auto *jc=chequeo.obtenerJugador();
  chequeo.saltar(); jc->setPos(3500,320); jc->iniciarVuelo(QPointF(0,100)); avanzar(chequeo,0.5);
  exigir(jc->estaEnSuelo(),"No aterriza en orilla intermedia");
  jc->setPos(3800,450); jc->establecerEnSuelo(false); chequeo.actualizar(paso); chequeo.actualizar(1.3);
  exigir(chequeo.vidasRestantes()==2 && jc->x()==3500 && jc->estaEnSuelo(),"Pierde checkpoint de lianas");
  cout<<"OK checkpoint de orilla intermedia"<<endl;
  exigir(h.items().size()==cuenta,"Crea objetos al jugar");
  for(int i=0;i<100;++i) h.reiniciar();
  exigir(h.items().size()==cuenta,"Crea objetos al reiniciar");
  cout<<"OK recorrido completo real y reutilizacion en cien reinicios"<<endl;
  // Integracion real: recoger llaves, recorrer carretera, Enter y controles de liana.
  MainWindow ventana; ventana.show(); QMetaObject::invokeMethod(&ventana, "elegirModo", Qt::DirectConnection); app.processEvents(); ventana.findChild<QTimer *>()->stop();
  auto *n=ventana.findChild<Nivel *>(); auto *vista=ventana.findChild<QGraphicsView *>();
  for(auto *g:n->obtenerGuardias()) g->setPos(3000,3000);
  for(QPointF posicion:{QPointF(90,680),QPointF(1280,95)}) {
    n->obtenerJugador()->setPos(posicion);
    for(int i=0;i<150;i++) n->actualizar(paso);
  }
  n->obtenerJugador()->setPos(1360,720); n->actualizar(0.001); app.processEvents();
  auto *c=ventana.findChild<Carretera *>(); exigir(c,"No entra en carretera");
  // Se valida transicion; el combate de carretera se verifico en la actualizacion previa.
  for(int i=0;i<120*70 && c->obtenerEstado()==Carretera::Conduciendo;++i) {
    for(auto *o:c->obtenerObstaculos()) o->setX(-3000);
    for(auto *b:c->obtenerProyectiles()) b->desactivar();
    c->activarDimensionador(); c->actualizar(paso);
  }
  exigir(c->obtenerEstado()==Carretera::Llegada,"Carretera no termina"); app.processEvents();
  exigir(!ventana.findChild<Hacienda *>(),"Enter anticipada abre hacienda");
  const int resistenciaLlegada=c->resistenciaActual();
  c->actualizar(60); app.processEvents();
  exigir(c->obtenerEstado()==Carretera::Llegada && c->resistenciaActual()==resistenciaLlegada
         && c->distanciaRecorrida()==1200,"No espera a salvo a los 1200 m");
  QKeyEvent anterior(QEvent::KeyPress,Qt::Key_F,Qt::NoModifier); QApplication::sendEvent(vista,&anterior);
  exigir(vista->scene()==c,"La tecla anterior todavia abre hacienda");
  QKeyEvent repetida(QEvent::KeyPress,Qt::Key_Return,Qt::NoModifier,QString(),true);
  QApplication::sendEvent(vista,&repetida); app.processEvents();
  exigir(vista->scene()==c,"Autorrepeticion dispara entrada");
  int entradas=0; QObject::connect(c,&Carretera::entradaHabilitada,[&]{++entradas;});
  QKeyEvent f(QEvent::KeyPress,Qt::Key_Return,Qt::NoModifier); QApplication::sendEvent(vista,&f);
  QApplication::sendEvent(vista,&f); // Doble pulsacion antes de procesar la cola.
  app.processEvents();
  exigir(entradas==1,"Entrada emitida mas de una vez");
  auto *hac=ventana.findChild<Hacienda *>(); exigir(hac && vista->scene()==hac,"Enter no abre hacienda");
  QKeyEvent espacio(QEvent::KeyPress,Qt::Key_Space,Qt::NoModifier); QApplication::sendEvent(vista,&espacio);
  exigir(hac->obtenerJugador()->velocidadVerticalActual()<0,"Espacio no salta");
  hac->actualizar(0.05);
  ventana.grab().save("capturas_lianas/sprites_ventana.png");
  QKeyEvent r(QEvent::KeyPress,Qt::Key_R,Qt::NoModifier); QApplication::sendEvent(vista,&r);
  exigir(vista->scene()==hac && hac->vidasRestantes()==3 && hac->obtenerJugador()->x()==100,"R no reinicia hacienda");
  // Teclado E: agarra y Espacio suelta; mantener E no reagarrara inmediatamente.
  auto *lj=hac->obtenerLianas()[0]; auto *pj=hac->obtenerJugador();
  QKeyEvent e(QEvent::KeyPress,Qt::Key_E,Qt::NoModifier);
  QApplication::sendEvent(vista,&espacio);
  pj->setPos(lj->puntoAgarre()-QPointF(28,8));
  QApplication::sendEvent(vista,&e); hac->actualizar(paso);
  exigir(hac->estaEnLiana(),"E no agarra");
  vista->centerOn(pj);
  ventana.grab().save("capturas_lianas/ventana_lianas.png");
  QKeyEvent espacioRepetido(QEvent::KeyPress,Qt::Key_Space,Qt::NoModifier,QString(),true);
  QApplication::sendEvent(vista,&espacioRepetido); exigir(hac->estaEnLiana(),"Autorrepeticion suelta liana");
  QApplication::sendEvent(vista,&espacio); exigir(!hac->estaEnLiana(),"Espacio no suelta");
  QApplication::sendEvent(vista,&r);
  QFocusEvent fuera(QEvent::FocusOut); QApplication::sendEvent(vista,&fuera);
  exigir(!ventana.findChild<QTimer *>()->isActive(),"No pausa al perder foco");
  QApplication::sendEvent(vista,&espacio); pj->setPos(lj->puntoAgarre()-QPointF(28,8));
  hac->actualizar(paso); exigir(!hac->estaEnLiana(),"E queda pegada al perder foco");
  cout<<"OK integracion Enter unica tras 1200 m, E, Espacio, R y pausa por foco"<<endl;
 } catch(const exception &e) { cerr<<e.what()<<endl; return 1; }
}
