#include "audiojuego.h"
#include <QSoundEffect>
#include <QAudioOutput>
#include <QAudioDevice>
#include <QMediaDevices>
#include <QDebug>
#include <QUrl>
using namespace std;

AudioJuego::AudioJuego(QObject *padre) : QObject(padre)
{
    const array<const char *, Sonido::Total> archivos{{"llave", "alarma", "disparo", "impacto",
        "escudo", "victoria", "derrota", "salto", "camuflaje"}};
    ultimoSonido.fill(-1000);
    reloj.start();
    for (int i = 0; i < Sonido::Total; ++i)
    {
        efectos[i] = new QSoundEffect(this);
        efectos[i]->setObjectName(archivos[i]);
        efectos[i]->setSource(QUrl(QString("qrc:/sonidos/%1.wav").arg(archivos[i])));
    }
    musica = new QMediaPlayer(this);
    salidaMusica = new QAudioOutput(this);
    musica->setAudioOutput(salidaMusica);
    connect(musica, SIGNAL(mediaStatusChanged(QMediaPlayer::MediaStatus)),
        this, SLOT(revisarEstadoMusica(QMediaPlayer::MediaStatus)));
    connect(musica, SIGNAL(errorChanged()), this, SLOT(informarErrorMusica()));
    const QAudioDevice dispositivo = QMediaDevices::defaultAudioOutput();
    qInfo() << "Audio Perry: salida" << dispositivo.description()
            << "| formato preferido" << dispositivo.preferredFormat();
    establecerVolumen(volumen);
    cargarPista(Sonido::Perry);
}

QSoundEffect *AudioJuego::obtenerEfecto(int evento) const
{
    return evento >= 0 && evento < Sonido::Total ? efectos[evento] : nullptr;
}

void AudioJuego::reproducir(int evento)
{
    QSoundEffect *efecto = obtenerEfecto(evento);
    if (!efecto || !habilitado || pausado || volumen == 0 || efecto->status() != QSoundEffect::Ready)
        return;
    const qint64 ahora = reloj.elapsed();
    if (ahora - ultimoSonido[evento] < 90) return;
    ultimoSonido[evento] = ahora;
    efecto->play();
    emit efectoReproducido(evento);
}

void AudioJuego::detenerEfectos()
{
    for (auto *efecto : efectos) efecto->stop();
}

void AudioJuego::establecerHabilitado(bool activo)
{
    habilitado = activo;
    if (!activo)
    {
        detenerEfectos();
        musica->pause();
    }
    else actualizarMusica();
}

void AudioJuego::establecerPausa(bool pausa)
{
    pausado = pausa;
    if (pausa)
    {
        detenerEfectos();
        musica->pause();
    }
    else actualizarMusica();
}

void AudioJuego::establecerVolumen(int porcentaje)
{
    volumen = qBound(0, porcentaje, 100);
    const float nivel = float(volumen) / 100;
    const float ganancia = nivel * nivel;
    for (auto *efecto : efectos) efecto->setVolume(0.75f * ganancia);
    salidaMusica->setVolume(0.36f * ganancia);
}

void AudioJuego::cargarPista(int nuevaPista)
{
    const array<const char *, Sonido::TotalPistas> archivos{{
        "sigilo.wav", "accion.wav", "perry.mp3", "popeye.mp3", "victoria_popeye.mp3"}};
    // Reinicia incluso cuando se solicita el mismo archivo.
    musica->stop();
    pista = nuevaPista;
    musicaTerminada = false;
    musica->setLoops(pista == Sonido::VictoriaPopeye ? QMediaPlayer::Once : QMediaPlayer::Infinite);
    const QUrl fuente(QString("qrc:/sonidos/%1").arg(archivos[pista]));
    if (musica->source() != fuente) musica->setSource(fuente);
    musica->setPosition(0);
    actualizarMusica();
    emit musicaIniciada(pista);
}

void AudioJuego::establecerMusica(int nuevaPista)
{
    if (nuevaPista < 0 || nuevaPista >= Sonido::VictoriaPopeye) return;
    // Si se elige una opcion antes de acabar el audio unico, espera a que termine.
    if (celebrando)
    {
        pistaPendiente = nuevaPista;
        return;
    }
    if (pista == nuevaPista) return;
    cargarPista(nuevaPista);
}

void AudioJuego::reiniciarMusica(int nuevaPista)
{
    if (nuevaPista < 0 || nuevaPista >= Sonido::VictoriaPopeye) return;
    celebrando = false;
    victoriaEmitida = false;
    pistaPendiente = -1;
    detenerEfectos();
    cargarPista(nuevaPista);
}

void AudioJuego::reiniciarPerry() { reiniciarMusica(Sonido::Perry); }

void AudioJuego::celebrarVictoriaPopeye()
{
    if (victoriaEmitida) return;
    victoriaEmitida = true;
    celebrando = true;
    pistaPendiente = -1;
    detenerEfectos();
    cargarPista(Sonido::VictoriaPopeye);
}

void AudioJuego::revisarEstadoMusica(QMediaPlayer::MediaStatus estado)
{
    if (estado != QMediaPlayer::EndOfMedia || !celebrando) return;
    celebrando = false;
    musicaTerminada = true;
    // Si sigue en el dialogo, queda en silencio al terminar el audio unico.
    const int siguiente = pistaPendiente;
    pistaPendiente = -1;
    if (siguiente >= 0) cargarPista(siguiente);
}

void AudioJuego::actualizarMusica()
{
    if (habilitado && !pausado && !musicaTerminada
        && musica->playbackState() != QMediaPlayer::PlayingState)
        musica->play();
}

void AudioJuego::informarErrorMusica()
{
    if (musica->error() != QMediaPlayer::NoError)
        qWarning() << "Audio Perry:" << musica->source() << musica->errorString();
}

void AudioJuego::sonarCamuflaje(bool activo)
{
    if (activo) reproducir(Sonido::Camuflaje);
}

// Silencio entre la victoria y el comienzo de la conversacion.
void AudioJuego::detenerMusica()
{
    celebrando = false;
    pistaPendiente = -1;
    musicaTerminada = true;
    musica->stop();
}
