#include "introduccion.h"
#include "configuracion.h"
#include "jugador.h"
#include "llave.h"
#include "mainwindow.h"
#include "nivel.h"
#include "villano.h"
#include <QApplication>
#include <QDir>
#include <QFocusEvent>
#include <QGraphicsRectItem>
#include <QGraphicsPixmapItem>
#include <QGraphicsView>
#include <QImage>
#include <QKeyEvent>
#include <QPainter>
#include <QTimer>
#include <QtMath>
#include <cmath>
#include <functional>
#include <iostream>
#include <stdexcept>

using namespace std;

namespace {
void exigir(bool condicion, const char *mensaje)
{
    if (!condicion) throw runtime_error(mensaje);
}
QPointF centro(const QGraphicsItem *objeto) { return objeto->sceneBoundingRect().center(); }
qreal distancia(const QPointF &a, const QPointF &b)
{
    return hypot(a.x() - b.x(), a.y() - b.y());
}
void colocarCentro(Jugador *jugador, const QPointF &punto)
{
    jugador->setPos(punto - jugador->boundingRect().center());
}
void avanzar(Nivel &nivel, qreal segundos)
{
    while (segundos > 1.0e-8) {
        const qreal paso = qMin(segundos, qreal(1.0 / 120.0));
        nivel.actualizar(paso);
        segundos -= paso;
    }
}
// Aislamiento de pruebas de objetivos: no es parte del juego.
void apartarGuardias(Nivel &nivel)
{
    for (Villano *guardia : nivel.obtenerGuardias()) guardia->setPos(3000, 3000);
}
void recogerAmbas(Nivel &nivel)
{
    apartarGuardias(nivel);
    nivel.obtenerJugador()->setPos(90, 680);
    avanzar(nivel, 1.3);
    nivel.obtenerJugador()->setPos(1280, 95);
    avanzar(nivel, 1.3);
    exigir(nivel.cantidadLlavesRecogidas() == 2, "No se recogieron ambas llaves");
}
void exigirCuerpoLibre(const Nivel &nivel, const QRectF &cuerpo)
{
    exigir(nivel.sceneRect().contains(cuerpo), "Cuerpo fuera del mapa");
    for (QGraphicsItem *item : nivel.items()) {
        if (item->zValue() == 3 && (dynamic_cast<QGraphicsRectItem *>(item) || dynamic_cast<QGraphicsPixmapItem *>(item))) {
            exigir(!cuerpo.intersects(item->sceneBoundingRect()), "Un cuerpo entro en una pared o mueble");
        }
    }
}
}

int main(int argc, char *argv[])
{
    QApplication aplicacion(argc, argv);
    const QString carpeta = argc > 1 ? QString::fromLocal8Bit(argv[1]) : QDir::currentPath();
    QDir().mkpath(carpeta);
    int aprobadas = 0;
    auto prueba = [&aprobadas](const char *nombre, const function<void()> &caso) {
        caso();
        cout << "OK " << ++aprobadas << " " << nombre << endl;
    };
    try {
        prueba("Sprites y velocidad; movimiento diagonal normalizado", [] {
            Nivel nivel;
            Jugador *jugador = nivel.obtenerJugador();
            exigir(nivel.obtenerGuardias().size() == 4, "Faltan guardias");
            exigir(!QPixmap(":/imagenes/perry.png").isNull(), "Falta el sprite");
            jugador->setPos(100, 100);
            jugador->establecerDireccion(QPointF(1,1));
            jugador->actualizar(0.1);
            exigir(qAbs(distancia(jugador->pos(), QPointF(100,100)) - 22) < 0.001, "Diagonal mas rapida");
            jugador->alternarCamuflaje();
            exigir(jugador->rapidezActual() < ConfiguracionCasa::rapidezJugador, "Camuflaje no es lento");
            exigir(jugador->boundingRect() == QRectF(0,0,32,40), "Cambio el cuerpo");
        });
        prueba("Vision frontal, espalda, paredes y camuflaje incluso en contacto", [] {
            Nivel nivel;
            Jugador *jugador = nivel.obtenerJugador();
            Villano guardia(&nivel, jugador, {{500,100},{580,100}});
            colocarCentro(jugador, {560,100});
            exigir(guardia.puedeVerJugador(), "No ve al agente delante");
            jugador->alternarCamuflaje();
            exigir(!guardia.puedeVerJugador(), "Reconoce al camuflado");
            colocarCentro(jugador, centro(&guardia));
            exigir(!guardia.puedeVerJugador(), "Reconoce al camuflado en contacto");
            jugador->alternarCamuflaje();
            colocarCentro(jugador, {490,100});
            exigir(!guardia.puedeVerJugador(), "Ve por la espalda");
            Villano pared(&nivel, jugador, {{420,100},{540,100}});
            colocarCentro(jugador, {540,100});
            exigir(!pared.puedeVerJugador(), "Ve a traves de paredes");
            exigir(qAbs(nivel.recortarRayo({420,100},{540,100}).x()-460) < 0.001, "Cono atraviesa pared");
        });
        prueba("C cancela inmediatamente persecucion y busqueda", [] {
            Nivel nivel;
            Jugador *jugador = nivel.obtenerJugador();
            Villano guardia(&nivel, jugador, {{500,100},{580,100}});
            colocarCentro(jugador, {560,100});
            guardia.actualizar(0.01);
            exigir(guardia.obtenerEstado() == Villano::Persiguiendo, "No persigue");
            jugador->alternarCamuflaje();
            exigir(guardia.obtenerEstado() == Villano::Patrullando, "C no cancela inmediatamente");
            guardia.actualizar(0.2);
            exigir(guardia.obtenerEstado() == Villano::Patrullando, "Reanuda persecucion disfrazado");
            jugador->alternarCamuflaje();
            colocarCentro(jugador, centro(&guardia) + QPointF(40,0));
            guardia.actualizar(0.11);
            exigir(guardia.obtenerEstado() == Villano::Persiguiendo, "No reconoce al quitar camuflaje");
            colocarCentro(jugador, {1300,700});
            guardia.actualizar(0.11);
            exigir(guardia.obtenerEstado() == Villano::Investigando, "No busca ultima posicion");
            jugador->alternarCamuflaje();
            exigir(guardia.obtenerEstado() == Villano::Patrullando, "C no cancela busqueda");
        });
        prueba("Rutas entre las seis habitaciones, llaves y salida sin atravesar muebles", [] {
            Nivel nivel;
            const QList<QPointF> puntos = {{116,120},{108,691},{1298,106},{1380,740},
                {540,300},{860,100},{1240,340},{900,680},{1200,460},{200,440}};
            QList<QPointF> ruta;
            for (const QPointF &origen : puntos) {
                exigirCuerpoLibre(nivel, QRectF(origen-QPointF(16,20), QSizeF(32,40)));
                for (const QPointF &destino : puntos) {
                    nivel.calcularRuta(origen, destino, ruta);
                    exigir(!ruta.isEmpty(), "Habitaciones desconectadas");
                    exigir(distancia(ruta.last(),destino) < 0.001, "Destino incorrecto");
                    QPointF previo = origen;
                    for (const QPointF &punto : ruta) {
                        const int pasos = qMax(1, qCeil(distancia(previo,punto)/4));
                        for (int i=0; i<=pasos; ++i) {
                            const QPointF p = previo + (punto-previo)*(qreal(i)/pasos);
                            exigirCuerpoLibre(nivel, QRectF(p-QPointF(16,20), QSizeF(32,40)));
                        }
                        previo = punto;
                    }
                }
            }
        });
        prueba("Cuatro patrullas sin atascarse; pausa con vision activa", [] {
            Nivel nivel;
            colocarCentro(nivel.obtenerJugador(),{3000,3000});
            nivel.obtenerJugador()->alternarCamuflaje();
            QList<QPointF> inicios;
            array<qreal,4> distancias{};
            for (Villano *g : nivel.obtenerGuardias()) inicios.append(centro(g));
            for (int i=0; i<2400; ++i) {
                nivel.actualizar(1.0/120);
                for (int k=0; k<4; ++k) {
                    Villano *g = nivel.obtenerGuardias().at(k);
                    exigirCuerpoLibre(nivel, g->sceneBoundingRect());
                    distancias[k] += distancia(centro(g), inicios[k]);
                    inicios[k] = centro(g);
                }
            }
            for (qreal d : distancias) exigir(d > 500, "Guardia atascado");
            Villano g(&nivel, nivel.obtenerJugador(), {{500,100},{580,100}});
            g.actualizar(1.0);
            const QPointF llegada = centro(&g);
            g.actualizar(0.2);
            exigir(centro(&g) == llegada, "No hace pausa");
            colocarCentro(nivel.obtenerJugador(), {610,100});
            if (nivel.obtenerJugador()->estaCamuflado()) nivel.obtenerJugador()->alternarCamuflaje();
            g.actualizar(0.11);
            exigir(g.obtenerEstado() == Villano::Persiguiendo, "Pausa desactiva vision");
        });
        prueba("Camuflaje de cuatro segundos, expiracion automatica y reloj real", [] {
            Nivel nivel;
            apartarGuardias(nivel);
            auto *jugador=nivel.obtenerJugador();
            jugador->alternarCamuflaje();
            exigir(qAbs(jugador->segundosCamuflaje()-4.0)<0.001, "Duracion inicial incorrecta");
            nivel.actualizar(3.9);
            exigir(jugador->estaCamuflado(), "Termina antes de cuatro segundos");
            exigir(qAbs(jugador->segundosCamuflaje()-0.1)<0.001, "El limite fisico alarga la habilidad");
            nivel.actualizar(0.1);
            exigir(!jugador->estaCamuflado() && jugador->segundosCamuflaje()==0,
                   "No vuelve automaticamente a agente");
            exigir(jugador->rapidezActual()==ConfiguracionCasa::rapidezJugador, "No recupera rapidez");
            jugador->setPos(nivel.obtenerGuardias().first()->pos());
            jugador->alternarCamuflaje();
            nivel.actualizar(4.01);
            exigir(nivel.obtenerEstado()==Nivel::Reiniciando, "Sigue inmune despues de expirar");
        });
        prueba("Contacto camuflado seguro; contacto como agente captura", [] {
            Nivel nivel;
            Jugador *jugador = nivel.obtenerJugador();
            jugador->alternarCamuflaje();
            jugador->setPos(nivel.obtenerGuardias().first()->pos());
            nivel.actualizar(0.001);
            exigir(nivel.obtenerEstado() == Nivel::EnJuego, "Captura estando camuflado");
            jugador->alternarCamuflaje();
            nivel.actualizar(0.001);
            exigir(nivel.obtenerEstado() == Nivel::Reiniciando, "No captura al agente");
            nivel.actualizar(2.0);
            exigir(jugador->pos() == QPointF(100,100) && !jugador->estaCamuflado(), "Reinicio incompleto");
        });
        prueba("Llaves: sin camuflaje, quieto 1,2 s; progreso cancelable y sin duplicados", [] {
            Nivel nivel;
            apartarGuardias(nivel);
            Jugador *jugador = nivel.obtenerJugador();
            jugador->setPos(90,680);
            jugador->alternarCamuflaje();
            avanzar(nivel,1.5);
            exigir(nivel.cantidadLlavesRecogidas() == 0, "Recoge camuflado");
            jugador->alternarCamuflaje();
            avanzar(nivel,0.6);
            exigir(nivel.cantidadLlavesRecogidas() == 0, "Recoge antes de tiempo");
            jugador->alternarCamuflaje();
            nivel.actualizar(0.01);
            jugador->alternarCamuflaje();
            avanzar(nivel,0.7);
            exigir(nivel.cantidadLlavesRecogidas() == 0, "No cancela al camuflarse");
            jugador->establecerDireccion({1,0});
            nivel.actualizar(0.01);
            jugador->establecerDireccion({0,0});
            avanzar(nivel,0.6);
            exigir(nivel.cantidadLlavesRecogidas() == 0, "No cancela al moverse");
            avanzar(nivel,0.7);
            exigir(nivel.cantidadLlavesRecogidas() == 1 && !nivel.alarmaActivada(), "Primera llave incorrecta");
            avanzar(nivel,1.5);
            exigir(nivel.cantidadLlavesRecogidas() == 1, "Cuenta llave repetida");
        });
        prueba("Alarma real, guardias acelerados, derrota y reutilizacion", [] {
            Nivel nivel;
            const auto guardias = nivel.obtenerGuardias();
            auto *jugador = nivel.obtenerJugador();
            const auto cantidad = nivel.items().size();
            recogerAmbas(nivel);
            exigir(nivel.alarmaActivada(), "No activa alarma");
            const qreal antes = nivel.segundosRestantes();
            nivel.actualizar(2.0);
            exigir(qAbs(nivel.segundosRestantes()-(antes-2)) < 0.001, "Alarma usa tiempo limitado");
            nivel.actualizar(35.0);
            exigir(nivel.obtenerEstado() == Nivel::Reiniciando, "No pierde por tiempo");
            nivel.actualizar(2.0);
            for (int i=0; i<20; ++i) { recogerAmbas(nivel); nivel.reiniciarCasa(); }
            exigir(nivel.items().size()==cantidad && nivel.obtenerJugador()==jugador
                && nivel.obtenerGuardias()==guardias, "Reinicio crea objetos");
            exigir(nivel.cantidadLlavesRecogidas()==0 && !nivel.alarmaActivada(), "No restablece objetivos");
            colocarCentro(jugador,{3000,3000});
            Villano g(&nivel,jugador,{{500,100},{580,100}});
            const QPointF inicio=centro(&g);
            g.actualizar(0.1);
            const qreal normal=distancia(inicio,centro(&g));
            g.restablecer(); g.establecerAlarma(true); g.actualizar(0.1);
            exigir(distancia(inicio,centro(&g))>normal*1.1, "Alarma no acelera");
        });
        prueba("Salida con dos llaves, unica senal y panel visible en camara", [] {
            Nivel nivel;
            int victorias=0;
            QObject::connect(&nivel,&Nivel::casaSuperada,[&] { ++victorias; });
            nivel.obtenerJugador()->setPos(1360,720);
            nivel.actualizar(0.001);
            exigir(nivel.obtenerEstado()==Nivel::EnJuego, "Sale sin llaves");
            recogerAmbas(nivel);
            nivel.obtenerJugador()->alternarCamuflaje();
            nivel.obtenerJugador()->setPos(1360,720);
            nivel.actualizar(0.001);
            exigir(nivel.obtenerEstado()==Nivel::Completado && victorias==1, "No completa casa");
            nivel.actualizar(1.0);
            exigir(victorias==1, "Victoria repetida");
            for (QGraphicsItem *item : nivel.items()) {
                if (item->zValue()==50) exigir(QRectF(480,260,960,540).contains(item->sceneBoundingRect()), "Aviso fuera de camara");
            }
        });
        prueba("Partida completa en ambos ordenes, con guardias activos y sin teletransportes", [] {
            for (int orden=0; orden<2; ++orden) {
                Nivel nivel;
                Jugador *jugador=nivel.obtenerJugador();
                // Al ir primero a la oficina, espera que la patrulla salga de su esquina.
                if (orden==1) avanzar(nivel,3.0);
                jugador->alternarCamuflaje();
                qreal duracion=orden==1 ? 3.0 : 0.0;
                auto paso = [&](qreal dt) {
                    nivel.actualizar(dt);
                    duracion+=dt;
                    if (nivel.obtenerEstado()==Nivel::Reiniciando) {
                        cerr << "  Perdio orden=" << orden << " t=" << duracion
                             << " pos=" << jugador->x() << "," << jugador->y()
                             << " llaves=" << nivel.cantidadLlavesRecogidas()
                             << " cam=" << jugador->segundosCamuflaje() << endl;
                    }
                    exigir(nivel.obtenerEstado()!=Nivel::Reiniciando, "Perdio durante recorrido completo");
                };
                auto caminar = [&](const QPointF &destino) {
                    QList<QPointF> ruta;
                    nivel.calcularRuta(centro(jugador),destino,ruta);
                    exigir(!ruta.isEmpty(), "Sin ruta en partida completa");
                    for (const QPointF &punto : ruta) {
                        int pasos=0;
                        while (distancia(centro(jugador),punto)>0.001 && nivel.obtenerEstado()==Nivel::EnJuego) {
                            if (!jugador->estaCamuflado()) jugador->alternarCamuflaje();
                            exigir(++pasos<10000, "Jugador atascado en recorrido");
                            const qreal restante=distancia(centro(jugador),punto);
                            jugador->establecerDireccion(punto-centro(jugador));
                            paso(qMin(qreal(1.0/120),restante/jugador->rapidezActual()));
                            exigirCuerpoLibre(nivel,jugador->sceneBoundingRect());
                        }
                    }
                    jugador->establecerDireccion({0,0});
                };
                const array<QPointF,2> destinos{{QPointF(108,691),QPointF(1298,106)}};
                for (int i=0; i<2; ++i) {
                    caminar(destinos[(i+orden)%2]);
                    // El piloto espera un hueco entre patrullas antes de quitarse el disfraz.
                    int espera=0;
                    for (;;) {
                        bool seguro=true;
                        for (Villano *g : nivel.obtenerGuardias())
                            if (distancia(centro(g),centro(jugador))<240) seguro=false;
                        if (seguro) break;
                        exigir(++espera<14400, "No existe ventana para recoger la llave");
                        if (!jugador->estaCamuflado()) jugador->alternarCamuflaje();
                        paso(1.0/120);
                    }
                    if (jugador->estaCamuflado()) jugador->alternarCamuflaje();
                    for (int j=0; j<150; ++j) paso(1.0/120);
                    exigir(nivel.cantidadLlavesRecogidas()==i+1, "No recogio llave en recorrido real");
                    jugador->alternarCamuflaje();
                }
                caminar({1380,740});
                exigir(nivel.obtenerEstado()==Nivel::Completado, "No llega al auto a tiempo");
                cout << "  Orden " << orden << ": " << duracion << " s, alarma restante "
                     << nivel.segundosRestantes() << " s" << endl;
            }
        });
        prueba("Teclado, foco, camara movil y capturas", [&] {
            MainWindow ventana;
            ventana.show(); ventana.findChild<Introduccion *>()->terminar(); QMetaObject::invokeMethod(&ventana, "elegirModo", Qt::DirectConnection); aplicacion.processEvents();
            auto *nivel=ventana.findChild<Nivel *>();
            auto *vista=ventana.findChild<QGraphicsView *>();
            auto *reloj=ventana.findChild<QTimer *>();
            exigir(nivel && vista && reloj, "Ventana incompleta");
            QKeyEvent c(QEvent::KeyPress,Qt::Key_C,Qt::NoModifier);
            QApplication::sendEvent(vista,&c);
            exigir(nivel->obtenerJugador()->estaCamuflado(), "Tecla C no funciona");
            QKeyEvent repetida(QEvent::KeyPress,Qt::Key_C,Qt::NoModifier,QString(),true);
            QApplication::sendEvent(vista,&repetida);
            exigir(nivel->obtenerJugador()->estaCamuflado(), "Autorepeticion cambia disfraz");
            QKeyEvent d(QEvent::KeyPress,Qt::Key_D,Qt::NoModifier);
            QApplication::sendEvent(vista,&d);
            QFocusEvent fuera(QEvent::FocusOut);
            QApplication::sendEvent(vista,&fuera);
            exigir(!reloj->isActive(), "No pausa");
            const QPointF antes=nivel->obtenerJugador()->pos();
            nivel->obtenerJugador()->actualizar(0.02);
            exigir(antes==nivel->obtenerJugador()->pos(), "Tecla pegada");
            QFocusEvent dentro(QEvent::FocusIn);
            QApplication::sendEvent(vista,&dentro);
            exigir(reloj->isActive(), "No reanuda");
            QKeyEvent r(QEvent::KeyPress,Qt::Key_R,Qt::NoModifier);
            QApplication::sendEvent(vista,&r);
            QMetaObject::invokeMethod(&ventana,"moverFigura",Qt::DirectConnection);
            exigir(ventana.grab().save(carpeta+"/inicio.png"), "No guarda captura");
            nivel->obtenerJugador()->alternarCamuflaje();
            nivel->obtenerJugador()->setPos(1300,700);
            QMetaObject::invokeMethod(&ventana,"moverFigura",Qt::DirectConnection);
            exigir(vista->mapToScene(vista->viewport()->rect().center()).x()>900, "Camara no sigue a Perry");
            exigir(ventana.grab().save(carpeta+"/garaje.png"), "No guarda captura");
            QImage mapa(1440,800,QImage::Format_ARGB32_Premultiplied);
            mapa.fill(Qt::transparent);
            QPainter pintor(&mapa);
            nivel->render(&pintor); pintor.end();
            exigir(mapa.save(carpeta+"/mapa.png"), "No guarda mapa");
        });
        cout << "TOTAL: " << aprobadas << " pruebas aprobadas" << endl;
        return 0;
    } catch (const exception &error) {
        cerr << "FALLO tras " << aprobadas << " pruebas: " << error.what() << endl;
        return 1;
    }
}
