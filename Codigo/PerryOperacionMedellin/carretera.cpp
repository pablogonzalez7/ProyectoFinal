#include "carretera.h"
#include "eventossonido.h"
#include "recursossprites.h"
#include "vehiculo.h"
#include "obstaculo.h"
#include "proyectil.h"
#include <QGraphicsEllipseItem>
#include <QGraphicsPixmapItem>
#include <QImage>
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
    segundoPerseguidor=new Vehiculo;
    segundoPerseguidor->establecerEnemigo(true);
    addItem(segundoPerseguidor); segundoPerseguidor->setZValue(2);
    segundoAviso=addLine(QLineF(),QPen(QColor(255,90,70),2,Qt::DashLine));
    segundoAviso->setZValue(1);
    const QPixmap hoja(":/imagenes/casas_edificios.png");
    for (int i=0; i<2; ++i) {
        const QPixmap celda=hoja.copy(i*hoja.width()/2,0,hoja.width()/2,hoja.height());
        // Quitar solo el margen transparente al cargar, nunca durante paint.
        const QImage imagen=celda.toImage();
        QRect visible;
        for (int y=0; y<imagen.height(); ++y)
            for (int x=0; x<imagen.width(); ++x)
                if (qAlpha(imagen.pixel(x,y))>20) visible=visible.united(QRect(x,y,1,1));
        spritesEdificios[i]=celda.copy(visible).scaled(168,178,Qt::KeepAspectRatio,Qt::SmoothTransformation);
    }
    for (auto &edificio : edificios) {
        edificio=new QGraphicsPixmapItem;
        edificio->setAcceptedMouseButtons(Qt::NoButton);
        edificio->setData(0,"decoracion_carretera");
        edificio->setZValue(0);
        addItem(edificio);
    }
    reiniciar();
}
void Carretera::establecerDificultad(Dificultad nueva) { dificultad=nueva; }
void Carretera::reiniciar() {
    distancia=0; desplazamiento=0; desplazamientoTextura=0; ultimoMetro=0; vehiculo->restablecer();
    vehiculo->setY(300); vehiculo->show();
    for(auto *o:obstaculos) o->show();
    perseguidor->show();
    perseguidor->restablecer(); perseguidor->setPos(cantidadPerseguidores()==2 ? 402 : 456,419);
    segundoPerseguidor->restablecer(); segundoPerseguidor->setPos(510,419);
    segundoPerseguidor->setVisible(cantidadPerseguidores()==2);
    segundoDisparo=4; segundoAviso->hide();
    for (int i=0; i<int(edificios.size()); ++i) colocarEdificio(i, -650+(i%3)*310);
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
    if(estado==Llegada) return "1200/1200 m | Llegaste a la hacienda.\nPulsa Enter una vez para bloquear la entrada y pasar al nivel 2.";
    const QString escudo=tiempoEscudo>0 ? "ACTIVO" : (recarga>0 ? QString("%1 s").arg(qCeil(recarga)) : "LISTO (E)");
    QString objetivo=enLodo ? "LODO: el perseguidor se acerca." : "Esquiva las barreras y los disparos.";
    return QString("%1 km/h | Perseguidores: %2 | Distancia: %3 m | Escudo: %4\n%5")
        .arg(qRound(vehiculo->velocidadMetrosSegundo()*3.6)).arg(cantidadPerseguidores())
        .arg(qCeil(separacion)).arg(escudo).arg(objetivo);
}
void Carretera::publicarInformacion() {
    const QString texto=informacionActual();
    if(texto!=ultimoIndicador) { ultimoIndicador=texto; emit informacionCambiada(texto); }
}
void Carretera::activarEscudo() {
    if(estado!=Conduciendo || recarga>0) return;
    tiempoEscudo=ConfiguracionCarretera::duracionEscudo;
    emit sonidoSolicitado(Sonido::Escudo);
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
    emit sonidoSolicitado(victoria ? Sonido::Victoria : Sonido::Derrota);
    estado=victoria ? Llegada : Derrota;
    if (!victoria) resistencia=0;
    vehiculo->establecerDireccion(0);
    avisoDisparo->hide(); segundoAviso->hide(); dibujoEscudo->hide();
    for(auto *p:proyectiles) p->desactivar();
    avisoFinal->setText(mensaje);
    avisoFinal->setPos((960-avisoFinal->boundingRect().width())/2,140); avisoFinal->show();
    esperaReinicio=1.5;
    if(victoria) {
        perseguidor->hide();
        segundoPerseguidor->hide();
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
    const qreal objetivo=qBound(qreal(260),vehiculo->x()-(cantidadPerseguidores()==2 ? 54 : 0),qreal(652));
    const qreal dx=qBound(-120*paso,objetivo-perseguidor->x(),120*paso);
    perseguidor->setPos(perseguidor->x()+dx,vehiculo->y()+84+separacion);
    if (cantidadPerseguidores()==2) {
        const qreal objetivo2=qBound(qreal(260),vehiculo->x()+54,qreal(652));
        segundoPerseguidor->setPos(segundoPerseguidor->x()+qBound(-120*paso,objetivo2-segundoPerseguidor->x(),120*paso),
                                  vehiculo->y()+84+separacion);
    }
    dibujoEscudo->setRect(vehiculo->sceneBoundingRect().adjusted(-8,-8,8,8));
    dibujoEscudo->setVisible(tiempoEscudo>0);
    if(separacion<=0) { finalizar(false,"Te alcanzaron!\nVolviendo a la casa..."); return; }
    actualizarDisparo(perseguidor,avisoDisparo,siguienteDisparo,destinoDisparo,paso);
    if (cantidadPerseguidores()==2)
        actualizarDisparo(segundoPerseguidor,segundoAviso,segundoDisparo,segundoDestino,paso);
    for(auto *p:proyectiles) {
        if(!p->estaActivo()) continue;
        p->actualizar(paso);
        const QRectF cuerpo=vehiculo->sceneBoundingRect();
        if(tiempoEscudo>0 && p->sceneBoundingRect().intersects(cuerpo.adjusted(-8,-8,8,8))) p->desactivar();
        else if(p->sceneBoundingRect().intersects(cuerpo)) {
            p->desactivar();
            if(inmunidad<=0) { --resistencia; inmunidad=0.8; emit sonidoSolicitado(Sonido::Impacto); }
            if(resistencia==0) { finalizar(false,"El carro quedo inutilizado!\nVolviendo a la casa..."); return; }
        } else if(!sceneRect().adjusted(-30,-30,30,30).intersects(p->sceneBoundingRect())) p->desactivar();
    }
}

void Carretera::actualizarDisparo(Vehiculo *policia, QGraphicsLineItem *aviso,
                                 qreal &espera, QPointF &destino, qreal paso)
{
    const qreal anterior=espera;
    espera-=paso;
    if (anterior>ConfiguracionCarretera::avisoDisparo && espera<=ConfiguracionCarretera::avisoDisparo)
        destino=vehiculo->sceneBoundingRect().center();
    const QPointF origen=policia->pos()+QPointF(24,0);
    aviso->setVisible(espera<=ConfiguracionCarretera::avisoDisparo);
    aviso->setLine(QLineF(origen,destino));
    if (espera<=0) {
        for (auto *bala : proyectiles) if (!bala->estaActivo()) {
            bala->activar(origen,destino); emit sonidoSolicitado(Sonido::Disparo); break;
        }
        espera=ConfiguracionCarretera::intervaloDisparo; aviso->hide();
    }
}
void Carretera::colocarEdificio(int indice, qreal altura)
{
    auto *azar=QRandomGenerator::global();
    const int tipo=azar->bounded(2); // Independiente del anterior: pueden repetirse casas o edificios.
    auto *edificio=edificios[indice];
    edificio->setPixmap(spritesEdificios[tipo]);
    edificio->setData(1,tipo);
    const qreal x=(indice<3 ? 20 : 766)+azar->bounded(24);
    edificio->setPos(x,altura);
}
void Carretera::actualizarEdificios(qreal avance)
{
    for (auto *edificio : edificios) edificio->moveBy(0,avance);
    for (int i=0; i<int(edificios.size()); ++i) {
        if (edificios[i]->y()<=540) continue;
        const int inicio=i<3 ? 0 : 3;
        qreal primero=0;
        for (int j=inicio; j<inicio+3; ++j)
            if (i!=j) primero=qMin(primero,edificios[j]->y());
        colocarEdificio(i,primero-260-QRandomGenerator::global()->bounded(120));
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
                emit sonidoSolicitado(Sonido::Impacto); objeto->marcarGolpe(); vehiculo->recibirImpacto(); ++choques;
            }
        }
        const qreal velocidadAnterior=vehiculo->rapidezAvance();
        vehiculo->actualizarVelocidad(paso,enLodo);
        const qreal avance=(velocidadAnterior+vehiculo->rapidezAvance())*0.5*paso;
        distancia+=avance/ConfiguracionCarretera::pixelesPorMetro;
        actualizarEdificios(avance);
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
            finalizar(true,"Llegaste a la hacienda!\nPulsa Enter para entrar al nivel 2.");
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
