#include "configuracion.h"
#include "jugador.h"
#include "mainwindow.h"
#include "nivel.h"
#include "villano.h"

#include <QApplication>
#include <QDir>
#include <QFocusEvent>
#include <QGraphicsView>
#include <QImage>
#include <QKeyEvent>
#include <QPainter>
#include <QPixmap>
#include <QTimer>
#include <QtMath>
#include <cmath>
#include <functional>
#include <iostream>
#include <stdexcept>

namespace {
void exigir(bool condicion, const char *mensaje)
{
    if (!condicion) {
        throw std::runtime_error(mensaje);
    }
}

QPointF centro(const QGraphicsItem *objeto)
{
    return objeto->sceneBoundingRect().center();
}

qreal distancia(const QPointF &a, const QPointF &b)
{
    const QPointF d = a - b;
    return std::hypot(d.x(), d.y());
}

void colocarCentro(Jugador *jugador, const QPointF &punto)
{
    jugador->setPos(punto - jugador->boundingRect().center());
}

void recogerAmbas(Nivel &nivel)
{
    nivel.obtenerJugador()->setPos(90, 430);
    nivel.actualizar(0.001);
    nivel.obtenerJugador()->setPos(800, 95);
    nivel.actualizar(0.001);
    exigir(nivel.cantidadLlavesRecogidas() == 2, "No se recogieron ambas llaves");
}

const QList<QRectF> paredesEsperadas = {
    {0,0,960,20}, {0,520,960,20}, {0,20,20,500}, {940,20,20,500},
    {300,20,24,160}, {300,280,24,240}, {620,20,24,320}, {620,440,24,80},
    {20,250,140,24}, {240,250,60,24}, {644,220,96,24}, {840,220,100,24}
};

void exigirCuerpoLibre(const QRectF &cuerpo)
{
    exigir(QRectF(0, 0, 960, 540).contains(cuerpo), "Cuerpo fuera del mapa");
    for (const QRectF &pared : paredesEsperadas) {
        exigir(!cuerpo.intersects(pared), "Un cuerpo entro en una pared");
    }
}
}

int main(int argc, char *argv[])
{
    QApplication aplicacion(argc, argv);
    const QString carpeta = argc > 1 ? QString::fromLocal8Bit(argv[1]) : QDir::currentPath();
    QDir().mkpath(carpeta);
    int aprobadas = 0;
    auto prueba = [&aprobadas](const char *nombre, const std::function<void()> &caso) {
        caso();
        ++aprobadas;
        std::cout << "OK " << nombre << std::endl;
    };

    try {
        prueba("Construccion, sprites y cambio de rapidez al camuflarse", [] {
            Nivel nivel;
            Jugador *jugador = nivel.obtenerJugador();
            exigir(nivel.obtenerGuardias().size() == 3, "Deben existir tres guardias");
            exigir(!QPixmap(":/imagenes/perry.png").isNull(), "Falta el recurso de Perry");
            jugador->setPos(400, 200);
            jugador->establecerDireccion(QPointF(1, 0));
            jugador->actualizar(0.1);
            const qreal avanceAgente = jugador->x() - 400;
            jugador->setPos(400, 200);
            jugador->alternarCamuflaje();
            jugador->actualizar(0.1);
            const qreal avanceCamuflado = jugador->x() - 400;
            exigir(avanceCamuflado > 0 && avanceCamuflado < avanceAgente, "Camuflaje no reduce la rapidez");
            exigir(jugador->boundingRect() == QRectF(0,0,32,40), "El camuflaje cambio la caja de colision");
            jugador->alternarCamuflaje();
            exigir(!jugador->estaCamuflado(), "No regreso al modo agente");
        });

        prueba("Cono: frente, espalda, alcance camuflado y paredes opacas", [] {
            Nivel nivel;
            Jugador *jugador = nivel.obtenerJugador();
            Villano guardia(&nivel, jugador, {QPointF(400,100), QPointF(550,100)});
            colocarCentro(jugador, QPointF(500,100));
            exigir(guardia.puedeVerJugador(), "No ve al agente enfrente");
            jugador->alternarCamuflaje();
            exigir(!guardia.puedeVerJugador(), "Reconoce demasiado lejos al camuflado");
            colocarCentro(jugador, QPointF(440,100));
            exigir(guardia.puedeVerJugador(), "El camuflaje dio invisibilidad total");
            colocarCentro(jugador, QPointF(350,100));
            exigir(!guardia.puedeVerJugador(), "Ve por la espalda");
            jugador->alternarCamuflaje();
            Villano trasPared(&nivel, jugador, {QPointF(260,120), QPointF(400,120)});
            colocarCentro(jugador, QPointF(350,120));
            exigir(!trasPared.puedeVerJugador(), "Detecta a traves de la pared");
            const QPointF fin = nivel.recortarRayo(QPointF(260,120), QPointF(400,120));
            exigir(qAbs(fin.x() - 300.0) < 0.001, "El cono no se corta en la pared");
        });

        prueba("Persecucion persiste con sombrero fuera; perder vision lleva a investigar", [] {
            Nivel nivel;
            Jugador *jugador = nivel.obtenerJugador();
            Villano guardia(&nivel, jugador, {QPointF(400,100), QPointF(550,100)});
            colocarCentro(jugador, QPointF(500,100));
            guardia.actualizar(0.01);
            exigir(guardia.obtenerEstado() == Villano::Persiguiendo, "No empezo la persecucion");
            jugador->alternarCamuflaje();
            guardia.actualizar(0.11);
            exigir(guardia.obtenerEstado() == Villano::Persiguiendo, "El camuflaje borro la persecucion");
            colocarCentro(jugador, QPointF(780,450));
            guardia.actualizar(0.11);
            exigir(guardia.obtenerEstado() == Villano::Investigando, "No investiga la ultima posicion vista");
            for (int i = 0; i < 100; ++i) {
                guardia.actualizar(0.05);
            }
            exigir(guardia.obtenerEstado() == Villano::Patrullando, "No retomo su patrulla");
            exigir(centro(&guardia).x() < 600, "Siguio la posicion escondida del jugador");
        });

        prueba("Rutas por los pasillos, comprobadas con el cuerpo completo", [] {
            Nivel nivel;
            const QList<QPointF> puntos = {{80,100},{180,330},{250,220},{400,80},{580,380},{780,180},{880,480}};
            QList<QPointF> ruta;
            ruta.reserve(ConfiguracionCasa::totalCeldas + 2);
            for (const QPointF &origen : puntos) {
                for (const QPointF &destino : puntos) {
                    nivel.calcularRuta(origen, destino, ruta);
                    exigir(!ruta.isEmpty(), "Falta una ruta entre habitaciones conectadas");
                    exigir(distancia(ruta.last(), destino) < 0.001, "La ruta no termina en el destino");
                    QPointF previo = origen;
                    for (const QPointF &punto : ruta) {
                        const int pasos = qMax(1, qCeil(distancia(previo, punto)));
                        for (int i = 0; i <= pasos; ++i) {
                            const QPointF p = previo + (punto - previo) * (qreal(i) / pasos);
                            exigirCuerpoLibre(QRectF(p - QPointF(16,20), QSizeF(32,40)));
                        }
                        previo = punto;
                    }
                }
            }
        });

        prueba("Patrulla cruza habitaciones sin atascarse ni atravesar paredes", [] {
            Nivel nivel;
            colocarCentro(nivel.obtenerJugador(), QPointF(2000,2000));
            const QPointF destino(880,460);
            Villano guardia(&nivel, nivel.obtenerJugador(), {QPointF(100,100), destino});
            bool llego = false;
            for (int i = 0; i < 6000 && !llego; ++i) {
                guardia.actualizar(1.0 / 120.0);
                exigirCuerpoLibre(guardia.sceneBoundingRect());
                llego = distancia(centro(&guardia), destino) < 3.0;
            }
            exigir(llego, "El guardia no llego a la otra habitacion");
        });

        prueba("Alarma aumenta la velocidad de patrulla sin ver a Perry", [] {
            Nivel nivel;
            colocarCentro(nivel.obtenerJugador(), QPointF(2000,2000));
            Villano guardia(&nivel, nivel.obtenerJugador(), {QPointF(400,100), QPointF(550,100)});
            const QPointF inicio = centro(&guardia);
            guardia.actualizar(0.1);
            const qreal normal = distancia(inicio, centro(&guardia));
            guardia.restablecer();
            guardia.establecerAlarma(true);
            guardia.actualizar(0.1);
            exigir(distancia(inicio, centro(&guardia)) > normal * 1.1, "La alarma no acelero al guardia");
            exigir(guardia.obtenerEstado() == Villano::Patrullando, "La alarma revelo al jugador oculto");
        });

        prueba("Llaves unicas; alarma solo con la segunda; reloj usa tiempo real", [] {
            Nivel nivel;
            nivel.obtenerJugador()->setPos(90,430);
            for (int i = 0; i < 5; ++i) {
                nivel.actualizar(0.001);
            }
            exigir(nivel.cantidadLlavesRecogidas() == 1, "La misma llave se conto varias veces");
            exigir(!nivel.alarmaActivada(), "La alarma empezo antes de tener ambas llaves");
            nivel.obtenerJugador()->setPos(800,95);
            nivel.actualizar(0.001);
            exigir(nivel.alarmaActivada(), "No se activo la alarma");
            const qreal antes = nivel.segundosRestantes();
            nivel.actualizar(2.0);
            exigir(qAbs(nivel.segundosRestantes() - (antes - 2.0)) < 0.001, "El limite de fisica alargo el reloj");
            nivel.actualizar(ConfiguracionCasa::duracionAlarma);
            exigir(nivel.obtenerEstado() == Nivel::Reiniciando, "No pierde al agotar el tiempo");
            nivel.actualizar(ConfiguracionCasa::demoraReinicio + 0.01);
            exigir(nivel.obtenerEstado() == Nivel::EnJuego && !nivel.alarmaActivada(), "No reinicio tras agotar el tiempo");
            exigir(nivel.cantidadLlavesRecogidas() == 0, "Quedaron llaves recogidas tras reiniciar");
        });

        prueba("Contacto captura, reinicio completo y reutilizacion de objetos", [] {
            Nivel nivel;
            Jugador *jugador = nivel.obtenerJugador();
            const auto guardias = nivel.obtenerGuardias();
            const auto cantidadItems = nivel.items().size();
            jugador->alternarCamuflaje();
            jugador->setPos(guardias.first()->pos());
            nivel.actualizar(0.001);
            exigir(nivel.obtenerEstado() == Nivel::Reiniciando, "El contacto no captura");
            nivel.actualizar(ConfiguracionCasa::demoraReinicio + 0.01);
            exigir(jugador->pos() == QPointF(100,100), "No volvio al inicio");
            exigir(!jugador->estaCamuflado(), "El reinicio dejo el camuflaje activado");
            for (int i = 0; i < 100; ++i) {
                recogerAmbas(nivel);
                nivel.reiniciarCasa();
            }
            exigir(nivel.obtenerJugador() == jugador, "Se recreo al jugador");
            exigir(nivel.obtenerGuardias() == guardias, "Se recrearon los guardias");
            exigir(nivel.items().size() == cantidadItems, "Se acumularon objetos al reiniciar");
        });

        prueba("Puerta bloqueada con 0/1 llaves; victoria unica con 2", [] {
            Nivel nivel;
            int victorias = 0;
            QObject::connect(&nivel, &Nivel::casaSuperada, [&victorias] { ++victorias; });
            nivel.obtenerJugador()->setPos(895,450);
            nivel.actualizar(0.001);
            exigir(nivel.obtenerEstado() == Nivel::EnJuego, "Salio sin llaves");
            nivel.obtenerJugador()->setPos(90,430);
            nivel.actualizar(0.001);
            nivel.obtenerJugador()->setPos(895,450);
            nivel.actualizar(0.001);
            exigir(nivel.obtenerEstado() == Nivel::EnJuego, "Salio con una sola llave");
            nivel.obtenerJugador()->setPos(800,95);
            nivel.actualizar(0.001);
            nivel.obtenerJugador()->setPos(895,450);
            nivel.actualizar(0.001);
            exigir(nivel.obtenerEstado() == Nivel::Completado && victorias == 1, "No completo la casa");
            nivel.actualizar(10.0);
            exigir(victorias == 1, "La salida emitio varias victorias");
        });

        prueba("Teclado C/R, autorepeticion y pausa al perder el foco", [&] {
            MainWindow ventana;
            ventana.show();
            aplicacion.processEvents();
            auto *nivel = ventana.findChild<Nivel *>();
            auto *vista = ventana.findChild<QGraphicsView *>();
            auto *reloj = ventana.findChild<QTimer *>();
            exigir(nivel && vista && reloj, "Faltan elementos de la ventana");
            QKeyEvent c(QEvent::KeyPress, Qt::Key_C, Qt::NoModifier);
            QApplication::sendEvent(vista, &c);
            exigir(nivel->obtenerJugador()->estaCamuflado(), "C no activa el camuflaje");
            QKeyEvent repetida(QEvent::KeyPress, Qt::Key_C, Qt::NoModifier, QString(), true);
            QApplication::sendEvent(vista, &repetida);
            exigir(nivel->obtenerJugador()->estaCamuflado(), "Autorepeticion alterna el sombrero");
            QKeyEvent d(QEvent::KeyPress, Qt::Key_D, Qt::NoModifier);
            QApplication::sendEvent(vista, &d);
            QFocusEvent fuera(QEvent::FocusOut);
            QApplication::sendEvent(vista, &fuera);
            exigir(!reloj->isActive(), "No pauso el temporizador");
            const QPointF posicion = nivel->obtenerJugador()->pos();
            nivel->obtenerJugador()->actualizar(0.02);
            exigir(posicion == nivel->obtenerJugador()->pos(), "Quedo una tecla pegada al perder foco");
            QFocusEvent dentro(QEvent::FocusIn);
            QApplication::sendEvent(vista, &dentro);
            exigir(reloj->isActive(), "No reanudo el temporizador");
            QKeyEvent r(QEvent::KeyPress, Qt::Key_R, Qt::NoModifier);
            QApplication::sendEvent(vista, &r);
            exigir(!nivel->obtenerJugador()->estaCamuflado(), "R no restablece el modo agente");
            exigir(ventana.grab().save(carpeta + "/casa_inicial.png"), "No se guardo la captura");
            recogerAmbas(*nivel);
            exigir(ventana.grab().save(carpeta + "/casa_alarma.png"), "No se guardo la captura de alarma");
        });

        std::cout << "TOTAL: " << aprobadas << " pruebas aprobadas" << std::endl;
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "FALLO tras " << aprobadas << " pruebas: " << error.what() << std::endl;
        return 1;
    }
}
