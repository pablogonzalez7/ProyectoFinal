#include "audiojuego.h"
#include "dificultad.h"
#include "jugador.h"
#include "mainwindow.h"
#include "nivel.h"
#include "villano.h"
#include <QApplication>
#include <QAudioOutput>
#include <QAudioDevice>
#include <QCheckBox>
#include <QComboBox>
#include <QDir>
#include <QFile>
#include <QFocusEvent>
#include <QGraphicsView>
#include <QKeyEvent>
#include <QMediaDevices>
#include <QPushButton>
#include <QSlider>
#include <QSoundEffect>
#include <QTimer>
#include <iostream>
#include <stdexcept>
using namespace std;

void exigir(bool condicion, const char* mensaje)
{
    if (!condicion)
        throw runtime_error(mensaje);
}
void colocar(Jugador* jugador, QPointF centro) { jugador->setPos(centro - jugador->boundingRect().center()); }
int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    try
    {
        for (Dificultad dificultad : { Dificultad::Facil, Dificultad::Normal, Dificultad::Dificil })
        {
            Nivel nivel;
            nivel.establecerDificultad(dificultad);
            const auto guardias = nivel.obtenerGuardias();
            const int objetos = nivel.items().size();
            Jugador* jugador = nivel.obtenerJugador();
            Villano guardia(&nivel, jugador, { { 500, 100 }, { 580, 100 } });
            guardia.establecerDificultad(dificultad);
            int avisos = 0;
            QObject::connect(&guardia, &Villano::jugadorDetectado, [&](const QPointF&) { ++avisos; });
            colocar(jugador, { 560, 100 });
            guardia.actualizar(0.01);
            exigir(guardia.obtenerEstado() == Villano::Persiguiendo, "No inicia persecucion");
            exigir(avisos == (dificultad == Dificultad::Dificil ? 1 : 0), "Avisos incorrectos");
            guardia.actualizar(0.11);
            exigir(avisos <= 1, "Repite aviso durante una persecucion");
            colocar(jugador, { 1300, 700 });
            guardia.actualizar(0.11);
            exigir(guardia.obtenerEstado()
                    == (dificultad == Dificultad::Facil ? Villano::Patrullando : Villano::Investigando),
                "No cambia la estrategia al perder vision");
            jugador->alternarCamuflaje();
            exigir(guardia.obtenerEstado() == Villano::Patrullando, "Camuflaje no cancela busqueda");
            guardia.recibirAviso({ 560, 100 });
            exigir(guardia.obtenerEstado() == Villano::Patrullando, "Aviso reconoce camuflaje");
            jugador->alternarCamuflaje();
            guardia.recibirAviso({ 560, 100 });
            exigir(guardia.obtenerEstado()
                    == (dificultad == Dificultad::Dificil ? Villano::Investigando : Villano::Patrullando),
                "No respeta avisos segun dificultad");
            if (dificultad == Dificultad::Dificil)
            {
                // Oculto fuera del mapa: no puede actualizar la ultima posicion observada.
                colocar(jugador, { 3000, 3000 });
                bool llegoAlAviso = false, revisoOtroSector = false;
                for (int i = 0; i < 7200; ++i)
                {
                    guardia.actualizar(1.0 / 120);
                    const QPointF diferencia = guardia.sceneBoundingRect().center() - QPointF(560,100);
                    const qreal distancia2 = QPointF::dotProduct(diferencia, diferencia);
                    if (distancia2 < 9) llegoAlAviso = true;
                    if (llegoAlAviso && distancia2 > 1225 && guardia.obtenerEstado() == Villano::Investigando)
                        revisoOtroSector = true;
                }
                exigir(revisoOtroSector, "Modo dificil no revisa sectores tras llegar al aviso");
                exigir(guardia.obtenerEstado() == Villano::Patrullando, "Busqueda no termina");
                exigir(nivel.sceneRect().contains(guardia.sceneBoundingRect()),
                    "Persigue posicion oculta fuera del mapa");
            }
            for (int i = 0; i < 30; ++i)
                nivel.reiniciarCasa();
            exigir(nivel.obtenerGuardias() == guardias && nivel.items().size() == objetos,
                "Reinicio crea objetos");
            exigir(nivel.dificultadActual() == dificultad, "Reinicio pierde dificultad");
        }
        cout << "OK estrategias, avisos, camuflaje, busqueda finita y reutilizacion" << endl;

        Nivel nivel;
        nivel.establecerDificultad(Dificultad::Dificil);
        colocar(nivel.obtenerJugador(), { 3000, 3000 });
        // Emision equivalente al avistamiento real comprobado arriba: prueba la conexion de la escena.
        nivel.obtenerGuardias().first()->jugadorDetectado({ 560, 100 });
        for (Villano* guardia : nivel.obtenerGuardias())
            exigir(guardia->obtenerEstado() == Villano::Investigando, "Escena no comparte aviso");
        int llaves = 0, alarmas = 0;
        QObject::connect(&nivel, &Nivel::sonidoSolicitado,
            [&](int evento)
            {
                if (evento == Sonido::Llave)
                    ++llaves;
                if (evento == Sonido::Alarma)
                    ++alarmas;
            });
        nivel.reiniciarCasa();
        for (Villano* guardia : nivel.obtenerGuardias())
            guardia->setPos(3000, 3000);
        nivel.obtenerJugador()->setPos(90, 680);
        for (int i = 0; i < 180; ++i)
            nivel.actualizar(1.0 / 120);
        exigir(llaves == 1 && alarmas == 0, "Alarma antes de recoger las dos llaves");
        nivel.obtenerJugador()->setPos(1280, 95);
        for (int i = 0; i < 180; ++i)
            nivel.actualizar(1.0 / 120);
        exigir(llaves == 2 && alarmas == 1, "No emite sonidos de llaves y alarma una vez");
        cout << "OK conexion entre guardias y eventos reales de llave/alarma" << endl;

        AudioJuego audio;
        exigir(audio.findChildren<QSoundEffect*>().size() == 9, "Cantidad de reproductores incorrecta");
        for (QSoundEffect* efecto : audio.findChildren<QSoundEffect*>())
        {
            QFile archivo(":" + efecto->source().path());
            exigir(archivo.open(QIODevice::ReadOnly), "Recurso de sonido ausente");
            const QByteArray datos = archivo.readAll();
            exigir(datos.size() > 44 && datos.left(4) == "RIFF" && datos.mid(8, 4) == "WAVE", "WAV invalido");
        }
        exigir(audio.obtenerEfecto(-1) == nullptr && audio.obtenerEfecto(Sonido::Total) == nullptr,
            "Evento invalido admitido");
        audio.establecerVolumen(300);
        exigir(audio.volumenActual() == 100, "No limita volumen alto");
        audio.establecerVolumen(-1);
        exigir(audio.volumenActual() == 0 && audio.obtenerSalidaMusica()->volume() == 0, "No limita volumen bajo");
        audio.establecerVolumen(50);
        exigir(qAbs(audio.obtenerEfecto(Sonido::Llave)->volume() - 0.1875) < 0.0001, "Ganancia incorrecta");
        audio.establecerMusica(Sonido::Accion);
        exigir(audio.pistaActual() == Sonido::Accion
                && audio.obtenerMusica()->loops() == QMediaPlayer::Infinite,
            "Pista no configurada en bucle");
        audio.establecerMusica(999);
        exigir(audio.pistaActual() == Sonido::Accion, "Pista invalida admitida");
        int sonidos = 0;
        QObject::connect(&audio, &AudioJuego::efectoReproducido, [&](int) { ++sonidos; });
        audio.establecerPausa(true);
        audio.reproducir(Sonido::Llave);
        audio.establecerHabilitado(false);
        audio.establecerPausa(false);
        audio.reproducir(Sonido::Llave);
        exigir(sonidos == 0, "Reproduce pausado o silenciado");
        for (auto* efecto : audio.findChildren<QSoundEffect*>())
            exigir(!efecto->isPlaying(), "No detiene sonido");
        cout << "OK recursos, volumen, bucles, pausa y silencio" << endl;
        cout << (QMediaDevices::audioOutputs().isEmpty() ? "OMITIDA escucha: entorno sin dispositivo de audio"
                                                         : "Escucha manual pendiente en Windows")
             << endl;

        MainWindow ventana;
        ventana.show(); QMetaObject::invokeMethod(&ventana, "elegirModo", Qt::DirectConnection);
        app.processEvents();
        for (auto* reloj : ventana.findChildren<QTimer*>())
            reloj->stop();
        auto* vista = ventana.findChild<QGraphicsView*>();
        auto* casa = qobject_cast<Nivel*>(vista->scene());
        auto* selector = ventana.findChild<QComboBox*>("selectorDificultad");
        auto* boton = ventana.findChild<QPushButton*>("nuevaPartida");
        auto* sonido = ventana.findChild<AudioJuego*>();
        exigir(casa && selector && boton && sonido, "Falta control de interfaz");
        selector->setCurrentIndex(2);
        exigir(casa->dificultadActual() == Dificultad::Normal, "Aplica dificultad antes de Nueva partida");
        boton->click();
        QMetaObject::invokeMethod(&ventana, "elegirModo", Qt::DirectConnection);
        exigir(casa->dificultadActual() == Dificultad::Dificil && casa->cantidadLlavesRecogidas() == 0,
            "Nueva partida no aplica dificultad");
        QKeyEvent tecla(QEvent::KeyPress, Qt::Key_R, Qt::NoModifier);
        QApplication::sendEvent(vista, &tecla);
        exigir(casa->dificultadActual() == Dificultad::Dificil, "R pierde dificultad");
        QKeyEvent silenciar(QEvent::KeyPress, Qt::Key_M, Qt::NoModifier);
        QApplication::sendEvent(vista, &silenciar);
        exigir(!sonido->estaHabilitado(), "M no silencia");
        ventana.findChild<QSlider*>("volumen")->setValue(25);
        exigir(sonido->volumenActual() == 25, "Deslizador no controla volumen");
        QFocusEvent salida(QEvent::FocusOut), regreso(QEvent::FocusIn);
        QApplication::sendEvent(vista, &salida);
        exigir(sonido->estaPausado(), "Perder foco no pausa audio");
        QApplication::sendEvent(vista, &regreso);
        exigir(!sonido->estaPausado(), "Recuperar foco no reanuda");
        for (int i = 0; i < 30; ++i)
        {
            boton->click();
            QMetaObject::invokeMethod(&ventana, "elegirModo", Qt::DirectConnection);
        }
        exigir(sonido->findChildren<QSoundEffect*>().size() == 9, "Reinicio duplica audio");
        const QString carpeta = argc > 1 ? QString::fromLocal8Bit(argv[1]) : "capturas";
        QDir().mkpath(carpeta);
        app.processEvents();
        exigir(ventana.grab().save(carpeta + "/audio_dificultad.png"), "No guarda captura");
        cout << "OK interfaz, aplicacion de dificultad, reinicio, teclado M y foco" << endl;
    }
    catch (const exception& error)
    {
        cerr << "ERROR: " << error.what() << endl;
        return 1;
    }
    return 0;
}
