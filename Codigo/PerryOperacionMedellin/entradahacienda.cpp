#include "entradahacienda.h"
#include "guardiaentrada.h"
#include "jugador.h"
#include "proyectil.h"
#include "recursossprites.h"
#include "configuracionhacienda.h"
#include <QGraphicsRectItem>
#include <QPainter>
#include <QtMath>
#include <cmath>
using namespace std;

EntradaHacienda::EntradaHacienda(QObject *padre)
    : QGraphicsScene(padre), jugador(new Jugador(nullptr)),
      zonaGolpe(new QGraphicsRectItem(jugador))
{
    setSceneRect(0,0,960,540);
    paisaje=RecursosSprites::cargar(":/imagenes/panorama_hacienda.png",QSize(1620,540));
    suelo=RecursosSprites::cargar(":/imagenes/suelo_hacienda.png",QSize(192,120));
    addItem(jugador);
    jugador->activarModoPlataformas();
    jugador->setZValue(5);
    zonaGolpe->setPen(Qt::NoPen);
    zonaGolpe->setBrush(Qt::NoBrush);
    guardias[0]=new GuardiaEntrada(550);
    guardias[1]=new GuardiaEntrada(760);
    for (auto *guardia : guardias) addItem(guardia);
    for (auto &bala : balas) { bala=new Proyectil; addItem(bala); }
    reiniciar();
}
void EntradaHacienda::reiniciar()
{
    estado=Disfrazado;
    corazones=3;
    sorpresa=0;
    inmunidad=0;
    duracionGolpe=0;
    recargaGolpe=0;
    esperaDerrota=0;
    golpeUsado=false;
    derrotaEmitida=false;
    esperaDisparo={1.5,2.15};
    jugador->restablecer();
    jugador->setOpacity(1);
    jugador->setPos(70,376);
    jugador->aterrizar(420);
    jugador->establecerDisfrazEntrada(true);
    for (auto *guardia : guardias) guardia->reiniciar();
    for (auto *bala : balas) bala->desactivar();
    emit controlesLiberados();
    publicarInformacion();
    update();
}
void EntradaHacienda::establecerDireccion(int horizontal)
{
    if (estado==Derrota || estado==Completada) horizontal=0;
    jugador->establecerDireccion(QPointF(qBound(-1,horizontal,1),0));
}
void EntradaHacienda::saltar()
{
    if (estado!=Derrota && estado!=Completada) jugador->saltar();
}
void EntradaHacienda::transformar()
{
    // Solo una transformacion por intento: repetir C no prolonga la sorpresa.
    if (estado!=Disfrazado) return;
    estado=Sorpresa;
    sorpresa=3;
    jugador->establecerDisfrazEntrada(false);
    for (auto *guardia : guardias) guardia->cambiarEstado(GuardiaEntrada::Sorprendido);
    publicarInformacion();
}
void EntradaHacienda::golpear()
{
    if (estado==Despejada) {
        if (jugador->x()<830 || !jugador->estaEnSuelo()) return;
        estado=Completada;
        establecerDireccion(0);
        emit controlesLiberados();
        publicarInformacion();
        emit entradaSuperada();
        return;
    }
    if ((estado!=Sorpresa && estado!=Combate) || recargaGolpe>0 || jugador->estaEnSuelo()) return;
    recargaGolpe=0.45;
    duracionGolpe=0.20;
    golpeUsado=false;
    if (!jugador->estaEnSuelo()) jugador->iniciarPatada();
    actualizarGolpe();
}
void EntradaHacienda::actualizarGolpe()
{
    // Solo una patada en el aire a la cabeza incapacita. El cuerpo no cuenta.
    if (duracionGolpe<=0 || golpeUsado || jugador->estaEnSuelo()) return;
    zonaGolpe->setRect(jugador->miraIzquierda() ? QRectF(-22,26,36,18) : QRectF(42,26,36,18));
    for (auto *guardia : guardias) {
        if (guardia->estaActivo() && zonaGolpe->collidesWithItem(guardia->obtenerCabeza())) {
            guardia->cambiarEstado(GuardiaEntrada::Inconsciente);
            golpeUsado=true;
            break;
        }
    }
    if (!guardias[0]->estaActivo() && !guardias[1]->estaActivo()) {
        estado=Despejada;
        for (auto *bala : balas) bala->desactivar();
        update();
    }
}
void EntradaHacienda::disparar(GuardiaEntrada *guardia)
{
    const int direccion=guardia->miraIzquierda() ? -1 : 1;
    const QPointF origen=guardia->pos()+QPointF(direccion<0 ? -7 : 39,34);
    for (auto *bala : balas) {
        if (bala->estaActivo()) continue;
        bala->setRotation(direccion>0 ? 90 : -90);
        bala->activar(origen,origen+QPointF(direccion*100,0));
        break;
    }
}
void EntradaHacienda::recibirImpacto()
{
    if (inmunidad>0 || estado==Derrota) return;
    --corazones;
    inmunidad=0.9; // Evita que dos balas simultaneas gasten varias vidas.
    if (corazones==0) {
        estado=Derrota;
        esperaDerrota=1.2;
        establecerDireccion(0);
        for (auto *bala : balas) bala->desactivar();
        emit controlesLiberados();
    }
}
void EntradaHacienda::actualizarPaso(qreal dt)
{
    inmunidad=qMax(qreal(0),inmunidad-dt);
    recargaGolpe=qMax(qreal(0),recargaGolpe-dt);
    duracionGolpe=qMax(qreal(0),duracionGolpe-dt);
    jugador->setOpacity(inmunidad>0 ? 0.55 : 1);
    jugador->actualizar(dt);
    jugador->setX(qBound(qreal(0),jugador->x(),qreal(904)));
    if (jugador->velocidadVerticalActual()>=0 && jugador->sceneBoundingRect().bottom()>=420)
        jugador->aterrizar(420);
    if (estado==Sorpresa) {
        sorpresa=qMax(qreal(0),sorpresa-dt);
        if (sorpresa<=1e-8) {
            estado=Combate;
            for (auto *guardia : guardias)
                if (guardia->estaActivo()) guardia->cambiarEstado(GuardiaEntrada::Atacando);
        }
    }
    for (int i=0;i<2;++i) {
        GuardiaEntrada *guardia=guardias[i];
        if (!guardia->estaActivo()) continue;
        guardia->seguir(jugador->sceneBoundingRect().center().x(),dt);
        guardia->actualizar(dt);
        if (estado==Combate) {
            esperaDisparo[i]-=dt;
            if (esperaDisparo[i]<=0) { disparar(guardia); esperaDisparo[i]+=1.5; }
        }
    }
    actualizarGolpe();
    for (auto *bala : balas) {
        if (!bala->estaActivo()) continue;
        bala->actualizar(dt);
        if (bala->collidesWithItem(jugador->obtenerImagen())) {
            bala->desactivar();
            recibirImpacto();
        } else if (!sceneRect().intersects(bala->sceneBoundingRect())) bala->desactivar();
    }
}
void EntradaHacienda::actualizar(qreal segundos)
{
    if (!isfinite(segundos) || segundos<=0 || estado==Completada) return;
    if (estado==Derrota) {
        esperaDerrota-=segundos;
        if (esperaDerrota<=0 && !derrotaEmitida) { derrotaEmitida=true; emit partidaPerdida(); }
        return;
    }
    qreal restante=qMin(segundos,qreal(0.05));
    while (restante>1e-9 && estado!=Derrota) {
        const qreal dt=qMin(restante,ConfiguracionHacienda::pasoMaximo);
        actualizarPaso(dt);
        restante-=dt;
    }
    publicarInformacion();
}
QString EntradaHacienda::informacionActual() const
{
    QString consejo;
    if (estado==Disfrazado) consejo="Acercate sin sombrero. C: transformarte y sorprenderlos una sola vez.";
    else if (estado==Sorpresa) consejo=QString("Sorprendidos: %1 s. Salta y pulsa F para dar una patada en la cabeza.").arg(sorpresa,0,'f',1);
    else if (estado==Combate) consejo="Esquiva los disparos saltando. F en el aire: patada a la cabeza; en el cuerpo no los vence.";
    else if (estado==Despejada) consejo="Escoltas inconscientes. Acercate al porton de la derecha y pulsa F.";
    else if (estado==Derrota) consejo="Sin corazones. Volviendo al inicio de la hacienda...";
    else consejo="Entrada superada. Entrando al combate contra Popeye...";
    const int vencidos=int(!guardias[0]->estaActivo())+int(!guardias[1]->estaActivo());
    return QString("Entrada vigilada | Corazones: %1/3 | Escoltas: %2/2\n%3").arg(corazones).arg(vencidos).arg(consejo);
}
void EntradaHacienda::publicarInformacion()
{
    const QString texto=informacionActual();
    if (texto!=ultimoTexto) { ultimoTexto=texto; emit informacionCambiada(texto); }
}
void EntradaHacienda::drawBackground(QPainter *p,const QRectF &)
{
    p->drawPixmap(QRectF(0,0,960,540),paisaje,QRectF(660,0,960,540));
    p->drawTiledPixmap(QRectF(0,420,960,120),suelo);
    const bool abierta=estado==Despejada || estado==Completada;
    p->fillRect(QRectF(870,270,75,150),QColor("#263d35"));
    p->setPen(QPen(abierta ? QColor("#a9efc4") : QColor("#b7b6a3"),5));
    p->drawRect(QRectF(870,270,75,150));
    if (!abierta) for (int x=880;x<945;x+=15) p->drawLine(x,275,x,420);
    p->fillRect(QRectF(655,225,290,35),QColor(255,249,222,225));
    p->setPen(QColor("#244e49"));
    p->setFont(QFont("Arial",12,QFont::Bold));
    p->drawText(QRectF(655,225,290,35),Qt::AlignCenter,abierta ? "ENTRADA LIBRE - F" : "ENTRADA VIGILADA");
}
