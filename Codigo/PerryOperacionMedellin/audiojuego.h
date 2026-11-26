#ifndef AUDIOJUEGO_H
#define AUDIOJUEGO_H
#include <QObject>
#include <QElapsedTimer>
#include <QMediaPlayer>
#include <array>
#include "eventossonido.h"
using namespace std;
class QSoundEffect;
class QAudioOutput;
class AudioJuego : public QObject
{
    Q_OBJECT
public:
    explicit AudioJuego(QObject *padre = nullptr);
    bool estaHabilitado() const { return habilitado; }
    bool estaPausado() const { return pausado; }
    int volumenActual() const { return volumen; }
    int pistaActual() const { return pista; }
    QSoundEffect *obtenerEfecto(int evento) const;
    QMediaPlayer *obtenerMusica() const { return musica; }
    QAudioOutput *obtenerSalidaMusica() const { return salidaMusica; }
public slots:
    void reproducir(int evento);
    void establecerHabilitado(bool activo);
    void establecerPausa(bool pausa);
    void establecerVolumen(int porcentaje);
    void establecerMusica(int nuevaPista);
    void reiniciarMusica(int nuevaPista);
    void reiniciarPerry();
    void celebrarVictoriaPopeye();
    void detenerMusica();
    void sonarCamuflaje(bool activo);
signals:
    void efectoReproducido(int evento);
    void musicaIniciada(int pista);
private slots:
    void revisarEstadoMusica(QMediaPlayer::MediaStatus estado);
    void informarErrorMusica();
private:
    void cargarPista(int nuevaPista);
    void actualizarMusica();
    void detenerEfectos();
    // Objetos hijos: Qt los libera al destruir AudioJuego.
    array<QSoundEffect *, Sonido::Total> efectos{};
    array<qint64, Sonido::Total> ultimoSonido{};
    // Un reproductor para todas las canciones: decodificacion progresiva del MP3.
    QMediaPlayer *musica;
    QAudioOutput *salidaMusica;
    QElapsedTimer reloj;
    bool habilitado = true, pausado = false;
    bool celebrando = false, victoriaEmitida = false, musicaTerminada = false;
    int volumen = 70, pista = Sonido::Perry, pistaPendiente = -1;
};
#endif
