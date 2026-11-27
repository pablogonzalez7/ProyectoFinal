#include "mainwindow.h"
#include "introduccion.h"
#include "audiojuego.h"
#include <QApplication>
#include <QStackedWidget>
#include <QListWidget>
#include <QTimer>
#include <QMediaPlayer>
#include <QDir>
#include <QKeyEvent>
#include <iostream>
#include <stdexcept>
using namespace std;
void exigir(bool bien,const char *mensaje){if(!bien)throw runtime_error(mensaje);}
void tecla(QWidget *w,QEvent::Type tipo,int codigo,bool repetida=false){
    QKeyEvent e(tipo,codigo,Qt::NoModifier,QString(),repetida);QApplication::sendEvent(w,&e);
}
int main(int argc,char **argv)
{
    QApplication app(argc,argv);
    QString carpeta=argc>1?argv[1]:"capturas_intro";QDir().mkpath(carpeta);
    try {
        MainWindow v;v.show();app.processEvents();
        auto *intro=v.findChild<Introduccion *>();
        auto *pantallas=v.findChild<QStackedWidget *>("pantallas");
        auto *lista=v.findChild<QListWidget *>("listaModos");
        exigir(intro && intro->estaActiva() && pantallas->currentWidget()==intro,"No inicia antes del menu");
        exigir(intro->hasFocus(),"La intro no recibe Enter");
        exigir(v.findChild<AudioJuego *>()->estaPausado(),"Se mezcla musica del juego con intro");
        QMetaObject::invokeMethod(&v,"elegirModo",Qt::DirectConnection);
        exigir(pantallas->currentWidget()==intro,"Puede iniciar partida durante intro");
        intro->findChild<QTimer *>()->stop();
        for(qreal t:{3.0,7.0,11.5,17.4,20.5,25.0,29.0,34.5,39.0,44.0,47.5}) {
            intro->avanzar(t-intro->tiempoActual());app.processEvents();
            exigir(v.grab().save(carpeta+QString("/intro_%1.png").arg(int(t*10),3,10,QChar('0'))),"No guarda captura");
        }
        intro->avanzar(1);app.processEvents();
        exigir(!intro->estaActiva() && pantallas->currentIndex()==0 && lista->hasFocus(),"No pasa al menu al terminar");
        exigir(!intro->findChild<QTimer *>()->isActive(),"Queda animacion oculta activa");
        exigir(intro->findChild<QMediaPlayer *>()->playbackState()==QMediaPlayer::StoppedState,"No detiene musica de intro");
        v.grab().save(carpeta+"/menu_despues.png");
        for(int codigo:{Qt::Key_Return,Qt::Key_Enter}) {
            MainWindow otra;otra.show();app.processEvents();
            auto *i=otra.findChild<Introduccion *>();auto *p=otra.findChild<QStackedWidget *>("pantallas");
            auto *l=otra.findChild<QListWidget *>("listaModos");
            tecla(i,QEvent::KeyPress,codigo);tecla(i,QEvent::KeyPress,codigo,true);
            tecla(i,QEvent::KeyRelease,codigo,true);
            exigir(i->estaActiva(),"Autorrepeticion salta antes de liberar");
            tecla(i,QEvent::KeyRelease,codigo);app.processEvents();
            exigir(p->currentIndex()==0 && l->hasFocus(),"Enter no salta al menu");
            exigir(!i->estaActiva(),"Intro sigue activa tras saltar");
            tecla(l,QEvent::KeyPress,Qt::Key_Down);tecla(l,QEvent::KeyRelease,Qt::Key_Down);
            exigir(l->currentRow()==2,"Flechas no responden despues de intro");
            tecla(l,QEvent::KeyPress,Qt::Key_Return);tecla(l,QEvent::KeyRelease,Qt::Key_Return);
            exigir(p->currentIndex()==1,"Enter del menu no inicia juego");
            QMetaObject::invokeMethod(&otra,"nuevaPartidaConfigurada",Qt::DirectConnection);
            exigir(p->currentIndex()==0 && !i->estaActiva(),"Nueva partida repite intro");
        }
        cout<<"OK intro: inicio, 48 segundos, salto con ambos Enter, foco, menu, audio detenido y nueva partida"<<endl;
    } catch(const exception &e){cerr<<e.what()<<endl;return 1;}
}
