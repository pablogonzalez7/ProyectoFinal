#include "desenlace.h"
#include "escobar.h"
#include "jugador.h"
#include "proyectil.h"
#include "arenapopeye.h"
#include "popeye.h"
#include "mainwindow.h"
#include "nivel.h"
#include "villano.h"
#include "carretera.h"
#include "obstaculo.h"
#include "hacienda.h"
#include "entradahacienda.h"
#include "guardiaentrada.h"
#include <QApplication>
#include "audiojuego.h"
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QElapsedTimer>
#include <QThread>
#include <QGraphicsView>
#include <QGraphicsRectItem>
#include <QTimer>
#include <QKeyEvent>
#include <QImage>
#include <QPainter>
#include <QDir>
#include <iostream>
#include <stdexcept>
#include <cmath>
using namespace std;
constexpr qreal paso = 1.0 / 120;
void exigir(bool valor, const char* texto)
{
    if (!valor)
        throw runtime_error(texto);
}
void avanzar(Desenlace& e, qreal tiempo)
{
    for (int i = 0; i < int(round(tiempo / paso)); ++i)
        e.actualizar(paso);
}
void capturar(QGraphicsScene& e, const QString& ruta)
{
    QImage foto(960, 540, QImage::Format_ARGB32_Premultiplied);
    foto.fill(Qt::transparent);
    QPainter p(&foto);
    e.render(&p);
    p.end();
    foto.save(ruta);
}
void prepararCombate(Desenlace& e, int vidas = 6)
{
    e.reiniciar(vidas);
    e.continuar();
    e.elegir(true);
    avanzar(e, 1.22);
}
int main(int argc, char **argv)
{
    QApplication app(argc,argv);
    try {
        QString carpeta = argc > 1 ? QString::fromLocal8Bit(argv[1]) : "capturas_musica";
        QDir().mkpath(carpeta);

        AudioJuego audioUnidad;
        audioUnidad.establecerHabilitado(false);
        auto *reproductor = audioUnidad.obtenerMusica();
        QElapsedTimer espera;
        espera.start();
        while (reproductor->duration() <= 0 && espera.elapsed() < 6000) {
            app.processEvents(); QThread::msleep(10);
        }
        exigir(reproductor->duration() > 42000 && reproductor->duration() < 44000, "No carga MP3 Perry desde recursos");
        reproductor->setPosition(8000);
        audioUnidad.establecerMusica(Sonido::Perry);
        exigir(reproductor->position() == 8000, "Misma pista reinicia posicion");
        audioUnidad.establecerPausa(true);
        audioUnidad.establecerPausa(false);
        exigir(reproductor->position() == 8000, "Pausa reinicia cancion");
        audioUnidad.reiniciarPerry();
        exigir(reproductor->position() == 0, "Reinicio no vuelve al principio");
        audioUnidad.reiniciarMusica(Sonido::Popeye);
        exigir(reproductor->loops() == QMediaPlayer::Infinite, "Popeye no repite");
        int celebraciones = 0;
        QObject::connect(&audioUnidad, &AudioJuego::musicaIniciada, [&](int pista) {
            if (pista == Sonido::VictoriaPopeye) ++celebraciones;
        });
        audioUnidad.celebrarVictoriaPopeye();
        audioUnidad.celebrarVictoriaPopeye();
        exigir(celebraciones == 1 && reproductor->loops() == QMediaPlayer::Once, "Victoria se repite");
        QMetaObject::invokeMethod(&audioUnidad, "revisarEstadoMusica", Qt::DirectConnection,
            Q_ARG(QMediaPlayer::MediaStatus,QMediaPlayer::EndOfMedia));
        audioUnidad.establecerPausa(true);
        audioUnidad.establecerPausa(false);
        audioUnidad.celebrarVictoriaPopeye();
        exigir(celebraciones == 1, "Victoria terminada vuelve a empezar");
        audioUnidad.reiniciarMusica(Sonido::Popeye);
        audioUnidad.celebrarVictoriaPopeye();
        exigir(celebraciones == 2, "Nueva partida no permite otra victoria");
        exigir(audioUnidad.findChildren<QMediaPlayer*>().size() == 1, "Duplica reproductores");
        cout << "OK MP3 real, bucles, continuidad, pausa, reinicio y victoria unica" << endl;
        MainWindow ventana;
        ventana.show();
        app.processEvents();
        ventana.findChild<QTimer*>()->stop();
        auto* audio = ventana.findChild<AudioJuego*>();
        audio->establecerHabilitado(false);
        int inicios = 0, victorias = 0;
        QObject::connect(audio, &AudioJuego::musicaIniciada, [&](int pista) {
            ++inicios;
            if (pista == Sonido::VictoriaPopeye) ++victorias;
        });
        exigir(audio->pistaActual() == Sonido::Perry, "No comienza con Perry");
        auto* nivel = ventana.findChild<Nivel*>();
        nivel->obtenerJugador()->setPos(nivel->obtenerGuardias().first()->pos());
        nivel->actualizar(paso);
        exigir(inicios == 1, "Captura no reinicia musica una vez");
        for (int i=0;i<360;++i) nivel->actualizar(paso);
        exigir(inicios == 1, "Reaparecer duplica reinicio de captura");
        auto* vista = ventana.findChild<QGraphicsView*>();
        auto irHastaArena = [&]() -> ArenaPopeye*
        {
            const int antesDeTransiciones = inicios;
            for (auto* g : nivel->obtenerGuardias())
                g->setPos(3000, 3000);
            for (QPointF lugar : { QPointF(90, 680), QPointF(1280, 95) })
            {
                nivel->obtenerJugador()->setPos(lugar);
                for (int i = 0; i < 150; ++i)
                    nivel->actualizar(paso);
            }
            nivel->obtenerJugador()->setPos(1360, 720);
            nivel->actualizar(paso);
            app.processEvents();
            auto* carretera = ventana.findChild<Carretera*>();
            exigir(carretera, "No entra a carretera");
            exigir(audio->pistaActual() == Sonido::Perry && inicios == antesDeTransiciones,
                "Carretera cambia o reinicia la cancion");
            for (int i = 0; i < 120 * 80 && carretera->obtenerEstado() == Carretera::Conduciendo; ++i)
            {
                for (auto* o : carretera->obtenerObstaculos())
                    o->setX(-3000);
                for (auto* b : carretera->obtenerProyectiles())
                    b->desactivar();
                carretera->actualizar(paso);
            }
            carretera->activarDimensionador();
            app.processEvents();
            auto* hacienda = ventana.findChild<Hacienda*>();
            exigir(hacienda, "No entra a hacienda");
            exigir(audio->pistaActual() == Sonido::Perry && inicios == antesDeTransiciones,
                "Rio cambia o reinicia la cancion");
            // Dos caidas conservan musica; solo agotar las tres vidas la reinicia.
            for (int caida=0; caida<3; ++caida) {
                hacienda->obtenerJugador()->setPos(620,650);
                hacienda->obtenerJugador()->establecerEnSuelo(false);
                for (int i=0;i<300;++i) hacienda->actualizar(paso);
                exigir(inicios == antesDeTransiciones + (caida==2 ? 1 : 0), "Reinicio de musica incorrecto al caer al agua");
            }
            hacienda->obtenerJugador()->setPos(4220, 320);
            hacienda->obtenerJugador()->establecerEnSuelo(false);
            for (int i = 0; i < 120; ++i)
                hacienda->actualizar(paso);
            app.processEvents();
            auto* entrada = ventana.findChild<EntradaHacienda*>();
            exigir(entrada, "No entra a escoltas");
            exigir(audio->pistaActual() == Sonido::Perry && inicios == antesDeTransiciones + 1,
                "Escoltas cambia o reinicia la cancion");
            entrada->transformar();
            for (auto* g : entrada->obtenerGuardias())
                g->cambiarEstado(GuardiaEntrada::Inconsciente);
            entrada->saltar();
            entrada->golpear();
            entrada->obtenerJugador()->setPos(835, 376);
            entrada->obtenerJugador()->aterrizar(420);
            entrada->golpear();
            app.processEvents();
            auto* arena = ventana.findChild<ArenaPopeye*>();
            exigir(arena, "No entra a arena");
            exigir(audio->pistaActual() == Sonido::Popeye && inicios == antesDeTransiciones + 2,
                "No cambia una vez a la musica de Popeye");
            return arena;
        };
        auto* arena = irHastaArena();
        arena->obtenerPopeye()->recibirDano(10);
        arena->actualizar(paso);
        app.processEvents();
        exigir(vista->scene() == arena, "Sale de la arena sin pulsar F");
        exigir(audio->pistaActual() == Sonido::VictoriaPopeye && victorias == 1, "No reproduce victoria una vez");
        for (int i=0;i<120;++i) arena->actualizar(paso);
        exigir(victorias == 1, "Repite victoria por fotograma");
        auto tecla = [&](int codigo, bool repetida = false)
        {
            QKeyEvent evento(QEvent::KeyPress, codigo, Qt::NoModifier, QString(), repetida);
            QApplication::sendEvent(vista, &evento);
        };
        tecla(Qt::Key_F);
        tecla(Qt::Key_F);
        app.processEvents();
        auto* final = ventana.findChild<Desenlace*>();
        exigir(final && vista->scene() == final, "F no conecta al desenlace");
        exigir(audio->pistaActual() == Sonido::VictoriaPopeye, "F corta el sonido de victoria");
        QMetaObject::invokeMethod(audio, "revisarEstadoMusica", Qt::DirectConnection,
            Q_ARG(QMediaPlayer::MediaStatus,QMediaPlayer::EndOfMedia));
        exigir(audio->pistaActual() == Sonido::Sigilo, "No retoma musica de dialogo al terminar victoria");
        exigir(ventana.findChildren<Desenlace*>().size() == 1, "Crea dos escenas al repetir F");
        tecla(Qt::Key_F, true);
        exigir(final->obtenerEstado() == Desenlace::Dialogo, "Autorrepeticion salta dialogo");
        tecla(Qt::Key_F);
        tecla(Qt::Key_2);
        avanzar(*final, 1.22);
        exigir(final->obtenerEstado() == Desenlace::Combate, "Teclas no aceptan trato");
        tecla(Qt::Key_E);
        tecla(Qt::Key_Space);
        exigir(final->escudoActivo() && !final->obtenerJugador()->estaEnSuelo(), "Teclado no activa defensa");
        // Perder mientras el desenlace esta activo reinicia la casa.
        final->reiniciar(1);
        tecla(Qt::Key_F);
        tecla(Qt::Key_2);
        avanzar(*final, 1.22);
        final->obtenerEscobar()->setPos(-2000, 388);
        QPointF centro = final->obtenerJugador()->sceneBoundingRect().center();
        final->obtenerProyectiles()[0]->activar(centro, centro + QPointF(100, 0));
        final->actualizar(paso);
        avanzar(*final, 2);
        app.processEvents();
        exigir(vista->scene() == nivel && nivel->obtenerEstado() == Nivel::EnJuego,
            "Derrota final no reinicia juego");
        // Recorrer las transiciones otra vez conserva la misma escena final.
        arena = irHastaArena();
        arena->obtenerPopeye()->recibirDano(10);
        arena->actualizar(paso);
        tecla(Qt::Key_F);
        app.processEvents();
        exigir(vista->scene() == final && ventana.findChildren<Desenlace*>().size() == 1,
            "No reutiliza desenlace en otra partida");
        tecla(Qt::Key_F);
        tecla(Qt::Key_1);
        avanzar(*final, 8);
        exigir(final->obtenerEstado() == Desenlace::Victoria, "Rechazar desde ventana no termina");
        ventana.grab().save(carpeta + "/ventana_final.png");
        tecla(Qt::Key_F);
        app.processEvents();
        exigir(vista->scene() == nivel && nivel->obtenerEstado() == Nivel::EnJuego,
            "Nueva partida no vuelve a casa");
        cout << "OK MainWindow: F, decision, habilidades, final y reinicio desde casa" << endl;

        cout << "OK musica integrada con capturas, vidas, transiciones y victoria" << endl;
    } catch(const exception &error) { cerr << "ERROR: " << error.what() << endl; return 1; }
    return 0;
}
