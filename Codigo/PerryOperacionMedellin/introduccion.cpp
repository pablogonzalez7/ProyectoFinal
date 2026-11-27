#include "introduccion.h"
#include "recursossprites.h"
#include <QApplication>
#include <QTimer>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QPainter>
#include <QPainterPath>
#include <QKeyEvent>
#include <QDebug>
#include <QtMath>

namespace {
qreal tramo(qreal t, qreal a, qreal b) { return qBound(0.0,(t-a)/(b-a),1.0); }
qreal suave(qreal t, qreal a, qreal b) { qreal u=tramo(t,a,b);return u*u*(3-2*u); }
}
Introduccion::Introduccion(QWidget *padre) : QWidget(padre)
{
    setObjectName("introduccion");
    setFocusPolicy(Qt::StrongFocus);
    laboratorio=QPixmap(":/imagenes/intro_laboratorio.png");
    hacienda=QPixmap(":/imagenes/intro_hacienda.png");
    doctor=RecursosSprites::cargar(":/imagenes/intro_doof.png",QSize(185,270));
    doctorAccion=RecursosSprites::cargar(":/imagenes/intro_doof_accion.png",QSize(185,270));
    // Mismos personajes y atlas del juego, preparados una sola vez.
    const QRect poses[]={QRect(194,916,210,146),QRect(467,914,210,146),QRect(745,895,215,160),QRect(1015,895,220,168)};
    for(int i=0;i<4;++i) {
        perry[i]=RecursosSprites::cargar(":/imagenes/perry.png",QSize(208,144),poses[i]);
        popeye[i]=RecursosSprites::cargar(":/imagenes/popeye.png",QSize(150,215),QRect(i*543,0,543,724));
        escobar[i]=RecursosSprites::cargar(":/imagenes/escobar.png",QSize(155,215),QRect(i*543,0,543,724));
    }
    for(int i=0;i<2;++i)
        policias[i]=RecursosSprites::cargar(":/imagenes/policias.png",QSize(95,155),QRect(97,i*443+17,248,407));
    timer=new QTimer(this);timer->setInterval(33);timer->setObjectName("relojIntroduccion");
    connect(timer,SIGNAL(timeout()),this,SLOT(animar()));
    musica=new QMediaPlayer(this);musica->setObjectName("musicaIntroduccion");
    salida=new QAudioOutput(this);salida->setVolume(0.55f);musica->setAudioOutput(salida);
    musica->setSource(QUrl("qrc:/sonidos/intro_original.wav"));
    connect(musica,SIGNAL(errorChanged()),this,SLOT(errorAudio()));
    qApp->installEventFilter(this);
}
void Introduccion::iniciar()
{
    segundos=0;activa=true;pausada=false;teclaSalto=0;
    musica->setPosition(0);musica->play();reloj.start();timer->start();setFocus();update();
}
void Introduccion::terminar()
{
    if(!activa)return;
    activa=false;timer->stop();musica->stop();teclaSalto=0;
    emit terminada();
}
void Introduccion::animar() { avanzar(reloj.restart()/1000.0); }
void Introduccion::avanzar(qreal dt)
{
    if(!activa || pausada)return;
    segundos+=qMax(0.0,dt);
    if(segundos>=48) { terminar();return; }
    update();
}
void Introduccion::errorAudio()
{
    qWarning()<<"Audio de introduccion:"<<musica->errorString();
    // Un dispositivo sin sonido nunca bloquea la cinematica ni el menu.
}
void Introduccion::keyPressEvent(QKeyEvent *e)
{
    if(!e->isAutoRepeat()) {
        if(e->key()==Qt::Key_Return || e->key()==Qt::Key_Enter)teclaSalto=e->key();
        if(e->key()==Qt::Key_M){silenciada=!silenciada;salida->setMuted(silenciada);update();}
    }
    e->accept();
}
void Introduccion::keyReleaseEvent(QKeyEvent *e)
{
    // Consumir tambien la liberacion evita que el mismo Enter elija un modo.
    if(!e->isAutoRepeat() && teclaSalto==e->key() && teclaSalto!=0)terminar();
    e->accept();
}
bool Introduccion::eventFilter(QObject *o,QEvent *e)
{
    if(activa && e->type()==QEvent::ApplicationStateChange) {
        if(QGuiApplication::applicationState()!=Qt::ApplicationActive) {
            pausada=true;timer->stop();musica->pause();
        } else {
            pausada=false;reloj.restart();timer->start();musica->play();
        }
    }
    return QWidget::eventFilter(o,e);
}
void Introduccion::dibujarActor(QPainter &p,const QPixmap &s,QPointF pies,qreal alto,qreal giro,qreal escala,bool espejo)
{
    if(s.isNull() || escala<=0)return;
    const qreal ancho=alto*s.width()/s.height();
    p.save();p.translate(pies);p.rotate(giro);p.scale(espejo?-escala:escala,escala);
    p.drawPixmap(QRectF(-ancho/2,-alto,ancho,alto),s,QRectF(s.rect()));p.restore();
}
void Introduccion::dibujarPortal(QPainter &p,QPointF centro,qreal escala)
{
    p.save();p.translate(centro);p.scale(escala,escala);
    QRadialGradient luz(0,0,125);luz.setColorAt(0,QColor("#060b38"));
    luz.setColorAt(.6,QColor("#592be3"));luz.setColorAt(.84,QColor("#67ffe9"));luz.setColorAt(1,QColor(42,255,226,0));
    p.setPen(Qt::NoPen);p.setBrush(luz);p.drawEllipse(QRectF(-95,-125,190,250));
    for(int i=0;i<6;++i){p.setPen(QPen(QColor(140,255,244,190-i*20),2+i%2));
        p.drawArc(QRectF(-75+i*8,-108+i*12,150-i*16,216-i*24),int(segundos*150+i*55)*16,235*16);}
    for(int i=0;i<18;++i){qreal a=i*2*M_PI/18+segundos*2;
        p.setBrush(QColor("#b6fff0"));p.drawEllipse(QPointF(qCos(a)*84,qSin(a)*118),2.5,2.5);}
    p.restore();
}
void Introduccion::dibujarLaboratorio(QPainter &p)
{
    const qreal t=segundos;
    p.drawPixmap(QRectF(0,42,960,540),laboratorio,QRectF(laboratorio.rect()));
    qreal px=205,py=495,dx=570,dy=492,giro=0,pgiro=0,escala=1;
    int pose=0;
    if(t<2)px=-80+285*suave(t,0,2);
    else if(t<10){py-=qSin(t*4)*2;giro=qSin(t*2)*2;}
    else if(t<13){qreal u=tramo(t,10,13);px=205+75*u+220*qSin(u*M_PI);py-=105*qSin(u*M_PI);pose=2;giro=20*qSin(u*M_PI);}
    else if(t<16){px=280;giro=-8+qSin(t*6)*5;}
    else if(t<19){qreal u=tramo(t,16,19);px=280+265*suave(t,16,17.4)-170*suave(t,17.4,19);
        py-=145*qSin(u*M_PI);pose=2;pgiro=-30*qSin(u*M_PI);dx=570+240*suave(t,17.1,19);giro=35*suave(t,17.1,19);}
    else if(t<22){px=375;dx=810;giro=22;}
    else if(t<27){qreal u=suave(t,22,26.7);px=375+410*u;py=495-180*u;escala=1-u;pgiro=720*u;pose=3;dx=590+220*(1-suave(t,22,24));giro=-15;}
    else {escala=0;dx=590;giro=-10;}
    if(t>19)dibujarPortal(p,QPointF(787,278),suave(t,19,20));
    // Sombras mantienen los pies en el escenario durante el combate.
    p.setPen(Qt::NoPen);p.setBrush(QColor(0,0,0,65));
    p.drawEllipse(QRectF(dx-48,485,96,12));if(escala>.1)p.drawEllipse(QRectF(px-43,490,86,10));
    dibujarActor(p,t<17.1?doctorAccion:doctor,QPointF(dx,dy),270,giro,1,t<17.1);
    dibujarActor(p,perry[pose==0?int(t*5)%2:pose==2?3:2],QPointF(px,py),110,pgiro,escala,false);
    // Destello de choque y boton pulsado: el portal se abre por accidente.
    for(qreal golpe:{11.5,17.4,19.0})if(qAbs(t-golpe)<.22){
        QPointF c=golpe==19?QPointF(890,337):QPointF(550,385);
        p.setPen(QPen(QColor("#fff38b"),5));
        for(int i=0;i<8;++i){qreal a=i*M_PI/4;p.drawLine(c+QPointF(qCos(a),qSin(a))*18,c+QPointF(qCos(a),qSin(a))*42);}
    }
    if(t>19 && t<22){p.setPen(QPen(QColor("#ffdf59"),3));
        for(int i=0;i<5;++i){qreal y=230+i*32;p.drawLine(QPointF(900,y),QPointF(920+qSin(t*30+i)*10,y-12));}}
}
void Introduccion::dibujarHacienda(QPainter &p)
{
    const qreal t=segundos;
    p.drawPixmap(QRectF(0,42,960,540),hacienda,QRectF(hacienda.rect()));
    const qreal paso=suave(t,41,46);
    qreal px=350-245*paso,py=498-40*paso;
    if(t<33){qreal u=suave(t,30,33);py=230+268*u;dibujarPortal(p,QPointF(350,240),(1-u)*.8);}
    p.setPen(Qt::NoPen);p.setBrush(QColor(0,0,0,55));
    p.drawEllipse(QRectF(660,486,90,12));p.drawEllipse(QRectF(773,486,100,12));
    dibujarActor(p,popeye[0],QPointF(700,490),190);
    dibujarActor(p,escobar[0],QPointF(825,490),195, t>33&&t<41?qSin(t*4)*1.2:0);
    if(t>42 && t<46.3){
        p.setPen(Qt::NoPen);p.setBrush(QColor("#24170f"));p.drawRect(QRectF(64,339,67,119));
    }
    if(t<46.3){
        p.save();p.setOpacity(1-tramo(t,45.5,46.3));
        qreal u=suave(t,37,41);
        dibujarActor(p,policias[int(t*6)%2],QPointF(500-80*u-245*paso,505-45*paso),163);
        dibujarActor(p,policias[int(t*6+1)%2],QPointF(210+60*u-245*paso,505-45*paso),163,0,1,true);
        dibujarActor(p,perry[int(t*5)%2],QPointF(px,py-(t>41?qAbs(qSin(t*9))*5:0)),99,0,1,t>=41);
        p.restore();
    }
    dibujarActor(p,policias[0],QPointF(590,482),145);
    dibujarActor(p,policias[1],QPointF(920,498),150);
    if(t>46.3){
        p.setPen(QPen(QColor("#ffed9c"),3));p.drawLine(QPointF(77,337),QPointF(61,326));
        p.drawLine(QPointF(126,337),QPointF(141,325));
    }
}
void Introduccion::dibujarSubtitulo(QPainter &p)
{
    struct Frase { qreal desde,hasta; const char *nombre; const char *texto; };
    static const Frase frases[]={
        {2,6,"DOOFENSHMIRTZ","¡Perry! Llegas justo a tiempo para admirar mi... ¡Desubicador-inador!"},
        {6,10,"DOOFENSHMIRTZ","Te enviará tan lejos que necesitarás dos mapas... ¡y un transbordo!"},
        {13,16,"DOOFENSHMIRTZ","¡Oye! ¡Los golpes después de la explicación!"},
        {19,23,"DOOFENSHMIRTZ","Ese botón no era... ¡Ay, no! ¡Olvidé elegir el destino!"},
        {24,28,"DOOFENSHMIRTZ","¡Perry! ¡La garantía no cubre viajes interdimensionales!"},
        {33,37,"PABLO ESCOBAR","¿Y este animal de dónde salió?"},
        {37,41.5,"PABLO ESCOBAR","Popeye, que lo encierren en la casa. Y pongan guardias."}
    };
    for(const auto &f:frases)if(segundos>=f.desde && segundos<f.hasta){
        p.setPen(Qt::NoPen);p.setBrush(QColor(5,13,24,238));p.drawRoundedRect(QRectF(44,548,872,96),14,14);
        QFont font("Arial");font.setPixelSize(15);font.setBold(true);p.setFont(font);p.setPen(QColor("#ffdc6c"));
        p.drawText(QRectF(64,557,830,20),Qt::AlignLeft,QString::fromUtf8(f.nombre));
        font.setPixelSize(22);p.setFont(font);p.setPen(Qt::white);
        p.drawText(QRectF(64,583,830,54),Qt::AlignLeft|Qt::AlignVCenter|Qt::TextWordWrap,QString::fromUtf8(f.texto));
    }
}
void Introduccion::paintEvent(QPaintEvent *)
{
    QPainter p(this);p.fillRect(rect(),QColor("#060d18"));
    p.scale(width()/960.0,height()/660.0);p.setRenderHint(QPainter::Antialiasing);p.setRenderHint(QPainter::SmoothPixmapTransform);
    if(segundos<28)dibujarLaboratorio(p);
    else if(segundos<30){
        p.fillRect(QRectF(0,42,960,540),QColor("#090b31"));
        for(int i=9;i>=0;--i)dibujarPortal(p,QPointF(480,300),.25+i*.28+qSin(segundos*4)*.08);
        dibujarActor(p,perry[2],QPointF(480,365),100,segundos*160,.8);
    } else dibujarHacienda(p);
    QFont font("Arial");font.setPixelSize(14);font.setBold(true);p.setFont(font);p.setPen(QColor("#8ae8e0"));
    p.drawText(QRectF(24,8,520,25),Qt::AlignVCenter,segundos<30?"DANVILLE  /  LABORATORIO DOOFENSHMIRTZ":"COLOMBIA  /  HACIENDA NÁPOLES");
    p.setPen(QColor("#f8e7aa"));p.drawText(QRectF(535,8,400,25),Qt::AlignRight|Qt::AlignVCenter,silenciada?"M: SONIDO APAGADO    ·    ENTER: SALTAR":"M: SILENCIAR    ·    ENTER: SALTAR");
    dibujarSubtitulo(p);
    if(segundos>42 && segundos<46){font.setPixelSize(25);p.setFont(font);p.setPen(QColor("#ffdf72"));
        p.drawText(QRectF(80,585,800,50),Qt::AlignCenter,"UNA MISIÓN FUERA DE LUGAR");}
    qreal negro=segundos<.7?1-tramo(segundos,0,.7):segundos>47?tramo(segundos,47,48):0;
    if(negro>0)p.fillRect(QRectF(0,0,960,660),QColor(0,0,0,int(negro*255)));
    p.fillRect(QRectF(0,656,960*tramo(segundos,0,48),4),QColor("#42dbc5"));
}
