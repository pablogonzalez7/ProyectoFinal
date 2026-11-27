#include "introduccion.h"
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
int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    try
    {
        QString carpeta = argc > 1 ? QString::fromLocal8Bit(argv[1]) : "capturas_desenlace";
        QDir().mkpath(carpeta);
        Desenlace e;
        auto* j = e.obtenerJugador();
        auto* escobar = e.obtenerEscobar();
        const auto objetos = e.items().size();
        const auto balas = e.obtenerProyectiles();
        int finales = 0, derrotas = 0;
        QObject::connect(&e, &Desenlace::juegoCompletado, [&] { ++finales; });
        QObject::connect(&e, &Desenlace::juegoPerdido, [&] { ++derrotas; });
        e.reiniciar(3);
        exigir(e.corazonesRestantes() == 3, "No conserva corazones de Popeye");
        e.elegir(true);
        exigir(e.obtenerEstado() == Desenlace::Dialogo, "Elige antes del dialogo");
        e.continuar();
        capturar(e, carpeta + "/decision.png");
        e.elegir(false);
        e.elegir(true);
        exigir(e.obtenerEstado() == Desenlace::Entrega, "Segunda decision cambia la primera");
        avanzar(e, 2);
        exigir(e.obtenerEstado() == Desenlace::Portal, "Rechazar no abre portal");
        capturar(e, carpeta + "/portal.png");
        avanzar(e, 6);
        exigir(
            e.obtenerEstado() == Desenlace::Victoria && finales == 1, "Rechazo no completa mision una vez");
        capturar(e, carpeta + "/victoria.png");
        avanzar(e, 2);
        exigir(finales == 1, "Final se repite");
        cout << "OK dialogo, decision unica, entrega, portal y victoria" << endl;
        e.reiniciar(6);
        e.continuar();
        e.elegir(true);
        avanzar(e, 1);
        for (auto* b : balas)
            exigir(!b->estaActivo(), "Dispara sin tiempo para reaccionar");
        capturar(e, carpeta + "/advertencia.png");
        avanzar(e, 0.23);
        exigir(e.obtenerEstado() == Desenlace::Combate, "Advertencia no termina");
        int activos = 0;
        for (auto* b : balas)
            if (b->estaActivo())
                ++activos;
        exigir(activos == 1, "Primer ataque ausente o duplicado");
        Proyectil vertical, diagonal;
        vertical.activar(QPointF(100, 100), QPointF(100, 0), 520);
        vertical.actualizar(.1);
        exigir(
            abs(vertical.x() - 100) < .001 && abs(vertical.y() - 48) < .001, "Disparo vertical incorrecto");
        diagonal.activar(QPointF(100, 100), QPointF(200, 200), 520);
        diagonal.actualizar(.1);
        exigir(abs(QLineF(QPointF(100, 100), diagonal.pos()).length() - 52) < .001,
            "Velocidad diagonal incorrecta");
        cout << "OK aviso de traicion y disparos vectoriales rapidos" << endl;
        // Se aisla la colision para comprobar cabeza, cuerpo y escudo.
        for (auto* b : balas)
            b->desactivar();
        e.activarEscudo();
        auto centro = j->sceneBoundingRect().center();
        balas[0]->activar(centro, centro + QPointF(100, 0), 520);
        e.actualizar(paso);
        exigir(e.corazonesRestantes() == 6 && !balas[0]->estaActivo(), "Escudo no bloquea");
        j->setPos(715, 416);
        j->aterrizar(460);
        e.golpear();
        exigir(e.obtenerEstado() == Desenlace::Combate, "Puno en suelo incapacita");
        j->setPos(715, 400);
        j->establecerEnSuelo(false);
        e.establecerDireccion(1);
        e.establecerDireccion(0);
        e.golpear();
        exigir(e.obtenerEstado() == Desenlace::Combate, "Golpe al torso incapacita");
        prepararCombate(e);
        j->setPos(715, 370);
        j->establecerEnSuelo(false);
        e.establecerDireccion(1);
        e.establecerDireccion(0);
        e.golpear();
        exigir(e.obtenerEstado() == Desenlace::Entrega && escobar->poseActual() == Escobar::Incapacitado,
            "Patada a cabeza no incapacita");
        avanzar(e, 4);
        exigir(e.obtenerEstado() == Desenlace::Victoria, "Traicion superada no termina");
        cout << "OK escudo, golpe al torso rechazado y patada a cabeza" << endl;
        // Partida por controles publicos: no se cambia salud ni posicion.
        e.reiniciar(6);
        e.continuar();
        e.elegir(true);
        int cuadros = 0;
        for (; cuadros < 120 * 30
            && (e.obtenerEstado() == Desenlace::Advertencia || e.obtenerEstado() == Desenlace::Combate);
            ++cuadros)
        {
            const qreal cabeza = escobar->obtenerCabeza()->sceneBoundingRect().center().x();
            const qreal distancia = cabeza - j->sceneBoundingRect().center().x();
            e.establecerDireccion(distancia > 45 ? 1 : distancia < -45 ? -1 : 0);
            if (abs(distancia) < 160 && j->estaEnSuelo())
                e.saltar();
            if (abs(distancia) < 90)
                e.golpear();
            for (auto* b : balas)
                if (b->estaActivo() && QLineF(b->pos(), j->sceneBoundingRect().center()).length() < 100)
                    e.activarEscudo();
            e.actualizar(paso);
        }
        cout << "Piloto final: estado=" << e.obtenerEstado() << " corazones=" << e.corazonesRestantes()
             << " tiempo=" << cuadros / 120.0 << endl;
        capturar(e, carpeta + "/combate_resuelto.png");
        exigir(e.obtenerEstado() == Desenlace::Entrega, "No vence con controles normales");
        avanzar(e, 5);
        exigir(e.obtenerEstado() == Desenlace::Victoria, "Piloto no llega al final");
        prepararCombate(e);
        escobar->setPos(-2000, 388);
        for (auto* b : balas)
            b->desactivar();
        for (int i = 0; i < 6; ++i)
        {
            centro = j->sceneBoundingRect().center();
            balas[0]->activar(centro, centro + QPointF(100, 0));
            e.actualizar(paso);
            if (i < 5)
                avanzar(e, 1);
        }
        exigir(e.obtenerEstado() == Desenlace::Derrota, "Seis impactos no agotan corazones");
        avanzar(e, 3);
        exigir(derrotas == 1, "Derrota repetida o ausente");
        for (int i = 0; i < 100; ++i)
            e.reiniciar(6);
        exigir(e.items().size() == objetos && e.obtenerProyectiles() == balas, "Reinicio crea mas objetos");
        cout << "OK derrota unica y cien reinicios reutilizando objetos" << endl;
        // Transiciones de la ventana; los combates se prueban por separado arriba.
        MainWindow ventana;
        ventana.show(); ventana.findChild<Introduccion *>()->terminar(); QMetaObject::invokeMethod(&ventana, "elegirModo", Qt::DirectConnection);
        app.processEvents();
        ventana.findChild<QTimer*>()->stop();
        auto* nivel = ventana.findChild<Nivel*>();
        auto* vista = ventana.findChild<QGraphicsView*>();
        auto irHastaArena = [&]() -> ArenaPopeye*
        {
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
            hacienda->obtenerJugador()->setPos(4220, 320);
            hacienda->obtenerJugador()->establecerEnSuelo(false);
            for (int i = 0; i < 120; ++i)
                hacienda->actualizar(paso);
            app.processEvents();
            auto* entrada = ventana.findChild<EntradaHacienda*>();
            exigir(entrada, "No entra a escoltas");
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
            return arena;
        };
        auto* arena = irHastaArena();
        arena->obtenerPopeye()->recibirDano(10);
        arena->actualizar(paso);
        app.processEvents();
        exigir(vista->scene() == arena, "Sale de la arena sin pulsar Enter");
        auto tecla = [&](int codigo, bool repetida = false)
        {
            QKeyEvent evento(QEvent::KeyPress, codigo, Qt::NoModifier, QString(), repetida);
            QApplication::sendEvent(vista, &evento);
        };
        tecla(Qt::Key_Return);
        tecla(Qt::Key_Return);
        app.processEvents();
        auto* final = ventana.findChild<Desenlace*>();
        exigir(final && vista->scene() == final, "Enter no conecta al desenlace");
        exigir(ventana.findChildren<Desenlace*>().size() == 1, "Crea dos escenas al repetir Enter");
        tecla(Qt::Key_Return, true);
        exigir(final->obtenerEstado() == Desenlace::Dialogo, "Autorrepeticion salta dialogo");
        tecla(Qt::Key_Return);
        tecla(Qt::Key_2);
        avanzar(*final, 1.22);
        exigir(final->obtenerEstado() == Desenlace::Combate, "Teclas no aceptan trato");
        tecla(Qt::Key_E);
        tecla(Qt::Key_Space);
        exigir(final->escudoActivo() && !final->obtenerJugador()->estaEnSuelo(), "Teclado no activa defensa");
        // Perder mientras el desenlace esta activo reinicia la casa.
        final->reiniciar(1);
        tecla(Qt::Key_Return);
        tecla(Qt::Key_2);
        avanzar(*final, 1.22);
        final->obtenerEscobar()->setPos(-2000, 388);
        centro = final->obtenerJugador()->sceneBoundingRect().center();
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
        tecla(Qt::Key_Return);
        app.processEvents();
        exigir(vista->scene() == final && ventana.findChildren<Desenlace*>().size() == 1,
            "No reutiliza desenlace en otra partida");
        tecla(Qt::Key_Return);
        tecla(Qt::Key_1);
        avanzar(*final, 8);
        exigir(final->obtenerEstado() == Desenlace::Victoria, "Rechazar desde ventana no termina");
        ventana.grab().save(carpeta + "/ventana_final.png");
        tecla(Qt::Key_Return);
        app.processEvents();
        exigir(vista->scene() == nivel && nivel->obtenerEstado() == Nivel::EnJuego,
            "Nueva partida no vuelve a casa");
        cout << "OK MainWindow: Enter, decision, habilidades, final y reinicio desde casa" << endl;
    }
    catch (const exception& error)
    {
        cerr << "ERROR: " << error.what() << endl;
        return 1;
    }
    return 0;
}
