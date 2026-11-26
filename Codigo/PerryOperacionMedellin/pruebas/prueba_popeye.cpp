#include "arenapopeye.h"
#include "popeye.h"
#include "jugador.h"
#include "proyectilcombate.h"
#include "mainwindow.h"
#include "nivel.h"
#include "villano.h"
#include "carretera.h"
#include "obstaculo.h"
#include "proyectil.h"
#include "hacienda.h"
#include "entradahacienda.h"
#include "guardiaentrada.h"
#include <QApplication>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsView>
#include <QTimer>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QImage>
#include <QPainter>
#include <QDir>
#include <iostream>
#include <cmath>
#include <stdexcept>
using namespace std;
constexpr qreal paso = 1.0 / 120;
void exigir(bool valor, const char* mensaje)
{
    if (!valor)
        throw runtime_error(mensaje);
}
void avanzar(ArenaPopeye& a, qreal segundos)
{
    for (int i = 0; i < int(round(segundos / paso)); ++i)
        a.actualizar(paso);
}
void capturar(ArenaPopeye& a, const QString& ruta)
{
    QImage imagen(960, 540, QImage::Format_ARGB32_Premultiplied);
    imagen.fill(Qt::transparent);
    QPainter p(&imagen);
    a.render(&p);
    p.end();
    imagen.save(ruta);
}
int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    try
    {
        const QString carpeta = argc > 1 ? QString::fromLocal8Bit(argv[1]) : "capturas_popeye";
        QDir().mkpath(carpeta);
        ArenaPopeye arena;
        auto* p = arena.obtenerPopeye();
        auto* j = arena.obtenerJugador();
        const auto cantidad = arena.items().size();
        const auto balas = arena.obtenerBalasEnemigas();
        capturar(arena, carpeta + "/arena_inicio.png");
        // Memoria: respuestas observadas y resultados son registros diferentes.
        const qreal antes = p->preferenciaAtaque(Superior, 1);
        for (int i = 0; i < 5; ++i)
        {
            ExperienciaDisparo e;
            e.identificador = i + 1;
            e.tipo = Directo;
            e.distancia = 1;
            e.observada = true;
            e.respuesta = -1;
            e.resultado = Fallo;
            p->aprender(e);
        }
        exigir(p->preferenciaAtaque(Superior, 1) > antes, "No aprende tendencia a saltar");
        p->percibir(QPointF(500, 428));
        int superiores = 0;
        for (int i = 0; i < 200; ++i)
            if (p->razonar().ataque == Superior)
                ++superiores;
        exigir(superiores > 140, "No usa las preferencias aprendidas");
        exigir(p->tacticaActual() == Popeye::Acercarse, "Cuatro fallos de cinco no acercan");
        ExperienciaDisparo e;
        e.identificador = 55;
        e.tipo = Inferior;
        e.distancia = 2;
        e.resultado = Acierto;
        p->aprender(e);
        p->aprender(e);
        exigir(
            p->memoriaActual().aciertos[Inferior][2] == 1, "Atribuye resultado a otro ataque o lo duplica");
        e.identificador = 56;
        e.tipo = Superior;
        e.distancia = 1;
        e.resultado = Bloqueado;
        e.observada = true;
        e.respuesta = 1;
        p->aprender(e);
        exigir(p->memoriaActual().intentos[Superior][1] == 0 && p->memoriaActual().evasiones.back() == -1,
            "Un bloqueo altera efectividad o evasiones");
        p->recibirDano(3);
        p->razonar();
        exigir(p->tacticaActual() == Popeye::Retirarse, "No se retira con dano alto");
        for (int i = 0; i < 482; ++i)
            p->actualizar(paso);
        p->razonar();
        exigir(p->retiradaRestante() == 0, "Encadena retirada antes del descanso");
        cout << "OK aprendizaje: " << superiores
             << "/200 superiores; atribucion, bloqueos y retirada limitada" << endl;
        p->reiniciar();
        p->percibir(QPointF(100, 430));
        const auto decision = p->razonar();
        exigir(p->puedeSaltarA(decision.destino), "Elige salto imposible");
        const QPointF inicio = p->pos();
        p->actuar(decision);
        exigir(p->pos() == inicio, "Salto teletransporta");
        p->actualizar(0.1);
        exigir(!p->estaEnSuelo() && p->pos() != inicio, "No sigue parabola");
        const QPointF previo = p->pos();
        p->recibirDano(1);
        exigir(p->pos() == previo && p->resistenciaActual() == Popeye::resistenciaMaxima - 1,
            "Dano en vuelo mueve o no afecta");
        for (int i = 0; i < 180; ++i)
            p->actualizar(paso);
        exigir(p->estaEnSuelo(), "No aterriza en superficie real");
        ProyectilCombate a, b, onda;
        a.activar(QPointF(100, 100), QPointF(400, 200), true, Superior, 1);
        b.activar(QPointF(100, 100), QPointF(400, 200), true, Superior, 2);
        for (int i = 0; i < 60; ++i)
        {
            a.actualizar(paso);
            b.actualizar(paso);
            a.observar(QPointF(800, -200));
        }
        exigir(a.pos() == b.pos(), "Proyectil persigue objetivo despues de disparar");
        onda.activar(QPointF(100, 200), QPointF(800, 200), true, Oscilatorio, 3);
        onda.actualizar(0.25);
        exigir(abs(onda.x() - 145) < 0.001 && abs(onda.y() - (200 + 62 * (cos(3) - 1))) < 0.001,
            "Oscilacion incorrecta");
        cout << "OK saltos alcanzables, dano en vuelo, trayectorias fijas y oscilacion" << endl;
        // Habilidades aisladas del enemigo: se aparta para medir sus duraciones.
        arena.reiniciar();
        p->setPos(-2000, 396);
        arena.activarAgilidad();
        arena.actualizar(paso);
        exigir(arena.agilidadActiva() && j->rapidezActual() == 350, "Agilidad no acelera");
        avanzar(arena, 10);
        exigir(arena.tieneFatiga() && j->rapidezActual() == 180, "No entra en fatiga");
        arena.activarAgilidad();
        exigir(!arena.agilidadActiva(), "Reactiva durante fatiga");
        avanzar(arena, 10);
        exigir(!arena.tieneFatiga() && j->rapidezActual() == 250, "Fatiga no termina");
        arena.activarAgilidad();
        exigir(!arena.agilidadActiva(), "Recarga termina antes de 40 s");
        avanzar(arena, 20.1);
        arena.activarAgilidad();
        exigir(arena.agilidadActiva(), "Recarga no termina");
        arena.reiniciar();
        p->setPos(-2000, 396);
        arena.activarEscudo();
        auto* bala = balas[0];
        auto centro = j->sceneBoundingRect().center();
        bala->activar(centro, centro + QPointF(100, 0), true, Directo, 1000);
        arena.actualizar(paso);
        exigir(arena.corazonesRestantes() == 5 && p->memoriaActual().bloqueados == 1,
            "Escudo no bloquea o no registra bloqueo");
        avanzar(arena, 1.1);
        arena.activarEscudo();
        exigir(!arena.escudoActivo(), "Escudo ignora recarga");
        cout << "OK agilidad 10 s, fatiga 10 s, recarga y escudo" << endl;
        arena.reiniciar();
        p->setPos(135, 396);
        j->setPos(70, 416);
        arena.golpear();
        exigir(p->resistenciaActual() == Popeye::resistenciaMaxima - 1, "Puno no inflige un punto");
        avanzar(arena, 0.16);
        exigir(p->resistenciaActual() == Popeye::resistenciaMaxima - 1, "Golpe hace dano por fotograma");
        arena.reiniciar();
        p->setPos(-2000, 396);
        QGraphicsSceneMouseEvent clic(QEvent::GraphicsSceneMousePress);
        clic.setButton(Qt::LeftButton);
        clic.setScenePos(QPointF(900, 300));
        QApplication::sendEvent(&arena, &clic);
        QApplication::sendEvent(&arena, &clic);
        int activos = 0;
        for (auto* tiro : arena.obtenerBalasJugador())
            if (tiro->estaActivo())
                ++activos;
        exigir(activos == 1, "Clic no dispara o no respeta intervalo");
        cout << "OK puno una vez y clic con intervalo de un segundo" << endl;
        // Combate completo: solo controles publicos, sin mover enemigos ni editar salud.
        arena.reiniciar();
        arena.activarAgilidad();
        QPointF anterior = p->sceneBoundingRect().center();
        int cuadros = 0;
        bool capturaIntermedia = false;
        for (; cuadros < 120 * 100 && arena.obtenerEstado() == ArenaPopeye::EnJuego; ++cuadros)
        {
            const QPointF enemigo = p->sceneBoundingRect().center();
            const QPointF velocidadObservada = (enemigo - anterior) / paso;
            anterior = enemigo;
            qreal destinoX = enemigo.x();
            if (j->sceneBoundingRect().bottom() > p->sceneBoundingRect().bottom() + 105 && enemigo.x() > 350
                && enemigo.x() < 610)
                destinoX = j->x() < 480 ? 245 : 695;
            const qreal diferencia = destinoX - j->sceneBoundingRect().center().x();
            arena.establecerDireccion(diferencia > 60 ? 1 : diferencia < -60 ? -1 : 0);
            if (j->estaEnSuelo())
                arena.saltar();
            const qreal viaje
                = qMin(qreal(0.65), QLineF(j->sceneBoundingRect().center(), enemigo).length() / 520);
            QPointF prediccion = enemigo + velocidadObservada * viaje;
            if (!p->estaEnSuelo())
                prediccion.ry() += 700 * viaje * viaje;
            prediccion.setY(qBound(qreal(100), prediccion.y(), qreal(428)));
            arena.dispararA(prediccion);
            if (abs(enemigo.x() - j->sceneBoundingRect().center().x()) < 90)
                arena.golpear();
            for (auto* tiro : balas)
                if (tiro->estaActivo() && QLineF(tiro->pos(), j->sceneBoundingRect().center()).length() < 110)
                    arena.activarEscudo();
            arena.activarAgilidad();
            arena.actualizar(paso);
            if (cuadros > 600 && !capturaIntermedia)
            {
                capturar(arena, carpeta + "/arena_combate.png");
                capturaIntermedia = true;
            }
        }
        cout << "Piloto: estado=" << arena.obtenerEstado() << " corazones=" << arena.corazonesRestantes()
             << " Popeye=" << p->resistenciaActual() << " tiempo=" << cuadros / 120.0 << " s" << endl;
        avanzar(arena, 1.5);
        exigir(p->estaEnSuelo() && j->estaEnSuelo(), "Victoria deja personajes flotando");
        capturar(arena, carpeta + "/arena_resultado.png");
        exigir(arena.obtenerEstado() == ArenaPopeye::Victoria,
            "Piloto no supera el combate con controles normales");
        // Derrota unica y memoria reiniciada, sin reconstruir objetos.
        arena.reiniciar();
        p->setPos(-2000, 396);
        int derrotas = 0;
        QObject::connect(&arena, &ArenaPopeye::partidaPerdida, [&] { ++derrotas; });
        for (int i = 0; i < 5; ++i)
        {
            centro = j->sceneBoundingRect().center();
            bala->activar(centro, centro + QPointF(100, 0), true, Directo, 2000 + i);
            arena.actualizar(paso);
            if (i < 4)
                avanzar(arena, 1.0);
        }
        exigir(arena.obtenerEstado() == ArenaPopeye::Derrota, "Cinco impactos no terminan pelea");
        arena.actualizar(1.3);
        arena.actualizar(2);
        exigir(derrotas == 1, "Derrota ausente o repetida");
        for (int i = 0; i < 100; ++i)
            arena.reiniciar();
        exigir(arena.items().size() == cantidad && arena.obtenerBalasEnemigas() == balas
                && p->memoriaActual().experiencias == 0,
            "Reinicio recrea objetos o conserva aprendizaje");
        cout << "OK victoria real, derrota unica, memoria y cien reinicios" << endl;
        MainWindow ventana;
        ventana.show(); QMetaObject::invokeMethod(&ventana, "elegirModo", Qt::DirectConnection);
        app.processEvents();
        ventana.findChild<QTimer*>()->stop();
        auto* nivel = ventana.findChild<Nivel*>();
        auto* vista = ventana.findChild<QGraphicsView*>();
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
        auto* pj = hacienda->obtenerJugador();
        pj->setPos(4220, 320);
        pj->establecerEnSuelo(false);
        for (int i = 0; i < 120; ++i)
            hacienda->actualizar(paso);
        app.processEvents();
        auto* entrada = ventana.findChild<EntradaHacienda*>();
        exigir(entrada && vista->scene() == entrada, "Lianas no conectan con escoltas");
        entrada->transformar();
        for (auto* g : entrada->obtenerGuardias())
            g->cambiarEstado(GuardiaEntrada::Inconsciente);
        entrada->saltar();
        entrada->golpear();
        entrada->obtenerJugador()->setPos(835, 376);
        entrada->obtenerJugador()->aterrizar(420);
        entrada->golpear();
        app.processEvents();
        auto* combate = ventana.findChild<ArenaPopeye*>();
        exigir(combate && vista->scene() == combate, "Porton no abre arena de Popeye");
        ventana.grab().save(carpeta + "/ventana_popeye.png");
        const QPoint local = vista->mapFromScene(QPointF(850, 420));
        QMouseEvent raton(QEvent::MouseButtonPress, QPointF(local),
            QPointF(vista->viewport()->mapToGlobal(local)), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        QApplication::sendEvent(vista->viewport(), &raton);
        activos = 0;
        for (auto* tiro : combate->obtenerBalasJugador())
            if (tiro->estaActivo())
                ++activos;
        exigir(activos == 1, "Clic en la ventana no dispara");
        QKeyEvent repetirQ(QEvent::KeyPress, Qt::Key_Q, Qt::NoModifier, QString(), true);
        QApplication::sendEvent(vista, &repetirQ);
        exigir(!combate->agilidadActiva(), "Autorrepeticion activa habilidad");
        for (int tecla : { Qt::Key_Q, Qt::Key_E, Qt::Key_Space })
        {
            QKeyEvent evento(QEvent::KeyPress, tecla, Qt::NoModifier);
            QApplication::sendEvent(vista, &evento);
        }
        exigir(
            combate->agilidadActiva() && combate->escudoActivo() && !combate->obtenerJugador()->estaEnSuelo(),
            "Teclado no llega a la arena");
        QKeyEvent reinicio(QEvent::KeyPress, Qt::Key_R, Qt::NoModifier);
        QApplication::sendEvent(vista, &reinicio);
        exigir(combate->corazonesRestantes() == 5 && !combate->agilidadActiva(), "R no reinicia pelea");
        combate->obtenerPopeye()->setPos(-2000, 396);
        for (int i = 0; i < 5; ++i)
        {
            auto centro = combate->obtenerJugador()->sceneBoundingRect().center();
            combate->obtenerBalasEnemigas()[0]->activar(
                centro, centro + QPointF(100, 0), true, Directo, 3000 + i);
            combate->actualizar(paso);
            if (i < 4)
                avanzar(*combate, 1.0);
        }
        combate->actualizar(1.3);
        app.processEvents();
        exigir(vista->scene() == hacienda && hacienda->obtenerJugador()->x() == 100
                && hacienda->vidasRestantes() == 3,
            "Perder ante Popeye no vuelve a la hacienda");
        cout << "OK porton, clic real, teclas, reinicio y derrota en MainWindow" << endl;
    }
    catch (const exception& e)
    {
        cerr << "ERROR: " << e.what() << endl;
        return 1;
    }
    return 0;
}
