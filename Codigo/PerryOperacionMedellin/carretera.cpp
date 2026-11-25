#include "carretera.h"
#include "recursossprites.h"
#include "vehiculo.h"
#include "obstaculo.h"
#include "proyectil.h"
#include <QGraphicsEllipseItem>
#include <QGraphicsLineItem>
#include <QGraphicsSimpleTextItem>
#include <QtMath>
#include <QPainter>
#include <cmath>
using namespace std;
Carretera::Carretera(QObject *padre):QGraphicsScene(padre),vehiculo(new Vehiculo) {
    // Se preparan una sola vez; se reutilizan al dibujar y reiniciar.
    texturaAsfalto=RecursosSprites::cargar(":/imagenes/asfalto_carretera.png",QSize(240,200));
    texturaLlano=RecursosSprites::cargar(":/imagenes/llano_carretera.png",QSize(240,200));
    setSceneRect(0,0,960,540); addItem(vehiculo); vehiculo->setZValue(2);
    for(auto &objeto:obstaculos) { objeto=new Obstaculo; addItem(objeto); }
    perseguidor=new Vehiculo; perseguidor->establecerEnemigo(true); addItem(perseguidor); perseguidor->setZValue(2);
    dibujoEscudo=addEllipse(QRectF(),QPen(QColor(80,218,255),3),QBrush(QColor(50,180,255,45)));
    dibujoEscudo->setZValue(3);
    avisoDisparo=addLine(QLineF(),QPen(QColor(255,90,70),2,Qt::DashLine)); avisoDisparo->setZValue(1);
    avisoFinal=addSimpleText(QString(),QFont("Arial",18,QFont::Bold)); avisoFinal->setBrush(Qt::white); avisoFinal->setZValue(20);
    for(auto &p:proyectiles) { p=new Proyectil; addItem(p); }
    reiniciar();
}
void Carretera::reiniciar() {
    distancia=0; desplazamiento=0; desplazamientoTextura=0; ultimoMetro=0; vehiculo->restablecer();
    vehiculo->setY(300); vehiculo->show();
    for(auto *o:obstaculos) o->show();
    perseguidor->show();
    perseguidor->restablecer(); perseguidor->setPos(456,419);
    estado=Conduciendo; resistencia=3; separacion=ConfiguracionCarretera::separacionInicial;
    tiempoEscudo=0; recarga=0; inmunidad=0; siguienteDisparo=3; esperaReinicio=0;
    dimensionador=false; reinicioEmitido=false; avisoFinal->hide(); dibujoEscudo->hide(); avisoDisparo->hide();
    for(auto *p:proyectiles) p->desactivar();
    choques=0; enLodo=false; siguienteTipo=0; aleatorio.seed(21);
    for(int i=0;i<ConfiguracionCarretera::cantidadObstaculos;++i)
        colocarObstaculo(obstaculos[i],-180-i*ConfiguracionCarretera::separacion);
    ultimoIndicador=informacionActual();
    update(); emit informacionCambiada(informacionActual());
}
void Carretera::establecerDireccion(int direccion) { vehiculo->establecerDireccion(direccion); }
QString Carretera::informacionActual() const {
    if(estado==Derrota) return "ESCAPE FALLIDO | Regresando al inicio de la casa...";
    if(estado==Llegada) return "1200/1200 m | Llegaste a la hacienda.\nPulsa F una vez para bloquear la entrada y pasar al nivel 2.";
    const QString escudo=tiempoEscudo>0 ? "ACTIVO" : (recarga>0 ? QString("%1 s").arg(qCeil(recarga)) : "LISTO (E)");
    QString objetivo=enLodo ? "LODO: el perseguidor se acerca." : "Esquiva las barreras y los disparos.";
    return QString("%1/%2 m | %3 km/h | Resistencia: %4/3 | Perseguidor: %5 m | Escudo: %6\n%7")
        .arg(int(distancia)).arg(int(ConfiguracionCarretera::longitudRecorrido))
        .arg(qRound(vehiculo->velocidadMetrosSegundo()*3.6)).arg(resistencia)
        .arg(qCeil(separacion)).arg(escudo).arg(objetivo);
}
void Carretera::publicarInformacion() {
    const QString texto=informacionActual();
    if(texto!=ultimoIndicador) { ultimoIndicador=texto; emit informacionCambiada(texto); }
}
void Carretera::activarEscudo() {
    if(estado!=Conduciendo || recarga>0) return;
    tiempoEscudo=ConfiguracionCarretera::duracionEscudo;
    recarga=ConfiguracionCarretera::recargaEscudo;
    dibujoEscudo->setRect(vehiculo->sceneBoundingRect().adjusted(-8,-8,8,8)); dibujoEscudo->show();
    publicarInformacion();
}
void Carretera::activarDimensionador() {
    if(estado!=Llegada || dimensionador) return;
    dimensionador=true;
    emit entradaHabilitada();
}
void Carretera::finalizar(bool victoria,const QString &mensaje) {
    if(estado!=Conduciendo) return;
    estado=victoria ? Llegada : Derrota; vehiculo->establecerDireccion(0);
    avisoDisparo->hide(); dibujoEscudo->hide();
    for(auto *p:proyectiles) p->desactivar();
    avisoFinal->setText(mensaje);
    avisoFinal->setPos((960-avisoFinal->boundingRect().width())/2,140); avisoFinal->show();
    esperaReinicio=1.5;
    if(victoria) {
        perseguidor->hide();
        for(auto *o:obstaculos) o->hide();
        vehiculo->setPos(456,440);
        emit haciendaAlcanzada();
    }
}
void Carretera::actualizarPersecucion(qreal paso) {
    using namespace ConfiguracionCarretera;
    tiempoEscudo=qMax(qreal(0),tiempoEscudo-paso);
    recarga=qMax(qreal(0),recarga-paso); inmunidad=qMax(qreal(0),inmunidad-paso);
    separacion=qBound(qreal(0),separacion+(vehiculo->velocidadMetrosSegundo()-velocidadPerseguidor)*paso,separacionMaxima);
    const qreal dx=qBound(-120*paso,vehiculo->x()-perseguidor->x(),120*paso);
    perseguidor->setPos(perseguidor->x()+dx,vehiculo->y()+84+separacion);
    dibujoEscudo->setRect(vehiculo->sceneBoundingRect().adjusted(-8,-8,8,8));
    dibujoEscudo->setVisible(tiempoEscudo>0);
    if(separacion<=0) { finalizar(false,"Te alcanzaron!\nVolviendo a la casa..."); return; }
    const qreal anterior=siguienteDisparo;
    siguienteDisparo-=paso;
    if(anterior>ConfiguracionCarretera::avisoDisparo && siguienteDisparo<=ConfiguracionCarretera::avisoDisparo)
        destinoDisparo=vehiculo->sceneBoundingRect().center();
    const QPointF origen=perseguidor->pos()+QPointF(24,0);
    avisoDisparo->setVisible(siguienteDisparo<=ConfiguracionCarretera::avisoDisparo);
    avisoDisparo->setLine(QLineF(origen,destinoDisparo));
    if(siguienteDisparo<=0) {
        for(auto *p:proyectiles) if(!p->estaActivo()) { p->activar(origen,destinoDisparo); break; }
        siguienteDisparo=intervaloDisparo; avisoDisparo->hide();
    }
    for(auto *p:proyectiles) {
        if(!p->estaActivo()) continue;
        p->actualizar(paso);
        const QRectF cuerpo=vehiculo->sceneBoundingRect();
        if(tiempoEscudo>0 && p->sceneBoundingRect().intersects(cuerpo.adjusted(-8,-8,8,8))) p->desactivar();
        else if(p->sceneBoundingRect().intersects(cuerpo)) {
            p->desactivar();
            if(inmunidad<=0) { --resistencia; inmunidad=0.8; }
            if(resistencia==0) { finalizar(false,"El carro quedo inutilizado!\nVolviendo a la casa..."); return; }
        } else if(!sceneRect().adjusted(-30,-30,30,30).intersects(p->sceneBoundingRect())) p->desactivar();
    }
}

void Carretera::colocarObstaculo(Obstaculo *objeto,qreal altura) {
    const int carril=aleatorio.bounded(3);
    const auto tipo=(siguienteTipo++%2==0) ? Obstaculo::Barrera : Obstaculo::Lodo;
    objeto->preparar(tipo,320+carril*160,altura);
}
void Carretera::actualizar(qreal segundos) {
    if (!isfinite(segundos)||segundos<=0) return;
    if(estado==Derrota) {
        esperaReinicio-=segundos;
        if(esperaReinicio<=0 && !reinicioEmitido) { reinicioEmitido=true; emit partidaPerdida(); }
        return;
    }
    if(estado==Llegada) return;
    qreal pendiente=qMin(segundos,qreal(0.05));
    while(pendiente>1.0e-9 && estado==Conduciendo) {
        const qreal paso=qMin(pendiente,qreal(1.0/120));
        vehiculo->actualizar(paso,calzada.adjusted(12,0,-12,0));
        enLodo=false;
        const QRectF cuerpo=vehiculo->sceneBoundingRect();
        for(auto *objeto:obstaculos) {
            if(!cuerpo.intersects(objeto->sceneBoundingRect())) continue;
            if(objeto->obtenerTipo()==Obstaculo::Lodo) enLodo=true;
            else if(!objeto->fueGolpeado()) {
                objeto->marcarGolpe(); vehiculo->recibirImpacto(); ++choques;
            }
        }
        const qreal velocidadAnterior=vehiculo->rapidezAvance();
        vehiculo->actualizarVelocidad(paso,enLodo);
        const qreal avance=(velocidadAnterior+vehiculo->rapidezAvance())*0.5*paso;
        distancia+=avance/ConfiguracionCarretera::pixelesPorMetro;
        desplazamiento=fmod(desplazamiento+avance,100.0);
        desplazamientoTextura=fmod(desplazamientoTextura+avance,texturaAsfalto.height());
        for(auto *objeto:obstaculos) objeto->moveBy(0,avance);
        for(auto *objeto:obstaculos) {
            if(objeto->y()>sceneRect().bottom()+20) {
                qreal primero=0;
                for(auto *otro:obstaculos) if(otro!=objeto) primero=qMin(primero,otro->y());
                colocarObstaculo(objeto,primero-ConfiguracionCarretera::separacion);
            }
        }
        actualizarPersecucion(paso);
        if(estado==Conduciendo && distancia>=ConfiguracionCarretera::longitudRecorrido) {
            distancia=ConfiguracionCarretera::longitudRecorrido;
            finalizar(true,"Llegaste a la hacienda!\nPulsa F para entrar al nivel 2.");
        }
        pendiente-=paso;
    }
    update(sceneRect());
    publicarInformacion();
}

void Carretera::drawBackground(QPainter *p,const QRectF &) {
    // El desplazamiento visual sigue el avance real del carro.
    p->drawTiledPixmap(sceneRect(),texturaLlano,QPointF(0,-desplazamientoTextura));
    if(estado==Llegada) {
        p->fillRect(QRectF(200,240,560,190),QColor(201,182,142));
        p->fillRect(QRectF(400,300,160,130),QColor(60,44,35));
        p->fillRect(QRectF(200,230,560,20),QColor(148,67,45));
        return;
    }
    p->fillRect(QRectF(214,0,532,540),QColor(121,115,99));
    p->drawTiledPixmap(calzada,texturaAsfalto,QPointF(0,-desplazamientoTextura));
    p->setPen(QPen(QColor(246,213,107),4)); p->drawLine(248,0,248,540); p->drawLine(712,0,712,540);
    p->setPen(Qt::NoPen);
    for(int fila=-1;fila<6;++fila) {
        const qreal y=fila*100+desplazamiento;
        p->setBrush(QColor(212,217,218)); p->drawRect(QRectF(397,y,6,48)); p->drawRect(QRectF(557,y,6,48));
    }
}
