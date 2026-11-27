#ifndef POPEYE_H
#define POPEYE_H
#include "personaje.h"
#include "memoriacombate.h"
#include <QRandomGenerator>
#include <vector>
#include <utility>
using namespace std;
class Popeye : public Personaje
{
public:
    static constexpr int resistenciaMaxima = 10;
    enum Tactica
    {
        Mantener,
        Acercarse,
        Retirarse
    };
    Popeye(const vector<QRectF>& superficies, const vector<QPointF>& destinos);
    QRectF boundingRect() const override;
    void actualizar(qreal segundos) override;
    void reiniciar(int vidasIniciales = resistenciaMaxima);
    void percibir(const QPointF& posicionJugador);
    DecisionPopeye razonar();
    void actuar(const DecisionPopeye& decision);
    void aprender(const ExperienciaDisparo& experiencia);
    void recibirDano(int cantidad);
    bool estaEnSuelo() const
    {
        return enSuelo;
    }
    bool necesitaMoverse() const
    {
        return enSuelo && esperaMovimiento <= 0 && resistencia > 0;
    }
    bool puedeSaltarA(int destino) const;
    int resistenciaActual() const
    {
        return resistencia;
    }
    void mostrarDisparo();
    qreal preferenciaAtaque(TipoAtaque tipo, int distancia) const;
    Tactica tacticaActual() const
    {
        return tactica;
    }
    const MemoriaCombate& memoriaActual() const
    {
        return memoria;
    }
    qreal retiradaRestante() const
    {
        return tiempoRetirada;
    }

private:
    qreal duracionSalto(const QPointF& destino) const;
    void actualizarSprite();
    // Referencias a datos de la arena, que vive mas tiempo que su personaje.
    const vector<QRectF>& superficies;
    const vector<QPointF>& destinos;
    array<array<QPixmap, 4>, 2> fotogramas;
    MemoriaCombate memoria;
    QRandomGenerator aleatorio { 73 };
    QPointF objetivo, velocidadSalto;
    bool enSuelo = true, izquierda = true;
    int resistencia = resistenciaMaxima;
    qreal reloj = 0, esperaMovimiento = 2, poseDisparo = 0;
    qreal tiempoRetirada = 0, bloqueoRetirada = 0;
    deque<pair<qreal, int>> danosRecientes;
    Tactica tactica = Mantener;
};
#endif
