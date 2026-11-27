#include "popeye.h"
#include "recursossprites.h"
#include <QTransform>
#include <QLineF>
#include <QtMath>
#include <algorithm>
#include <cmath>
using namespace std;
Popeye::Popeye(const vector<QRectF>& pisos, const vector<QPointF>& puntos)
    : Personaje(0)
    , superficies(pisos)
    , destinos(puntos)
{
    const QPixmap hoja(":/imagenes/popeye.png");
    for (int i = 0; i < 4; ++i)
    {
        fotogramas[0][i] = RecursosSprites::cargar(":/imagenes/popeye.png", QSize(44, 64),
            QRect(i * hoja.width() / 4, 0, hoja.width() / 4, hoja.height()));
        fotogramas[1][i] = fotogramas[0][i].transformed(QTransform().scale(-1, 1));
    }
    setZValue(5);
    setAcceptedMouseButtons(Qt::NoButton);
    reiniciar();
}
QRectF Popeye::boundingRect() const
{
    return QRectF(0, 0, 44, 64);
}
void Popeye::reiniciar(int vidasIniciales)
{
    memoria = MemoriaCombate();
    danosRecientes.clear();
    aleatorio.seed(73);
    resistencia = qBound(1, vidasIniciales, resistenciaMaxima);
    reloj = 0;
    esperaMovimiento = 2;
    poseDisparo = 0;
    tiempoRetirada = 0;
    bloqueoRetirada = 0;
    tactica = Mantener;
    enSuelo = true;
    izquierda = true;
    velocidadSalto = QPointF();
    objetivo = QPointF(100, 438);
    setRotation(0);
    setOpacity(1);
    setPos(838, 396);
    actualizarSprite();
}
void Popeye::percibir(const QPointF& posicionJugador)
{
    objetivo = posicionJugador;
    izquierda = objetivo.x() < x() + 22;
}
qreal Popeye::duracionSalto(const QPointF& destino) const
{
    const qreal diferencia = destino.y() - (y() + 64);
    const qreal discriminante = 540 * 540 + 2 * 1400 * diferencia;
    return discriminante >= 0 ? (540 + sqrt(discriminante)) / 1400 : -1;
}
bool Popeye::puedeSaltarA(int destino) const
{
    if (destino < 0 || destino >= int(destinos.size()))
        return false;
    const qreal duracion = duracionSalto(destinos[destino]);
    const qreal distancia = abs(destinos[destino].x() - (x() + 22));
    return duracion > 0 && distancia / duracion <= 420
        && (distancia > 35 || abs(destinos[destino].y() - (y() + 64)) > 5);
}
qreal Popeye::preferenciaAtaque(TipoAtaque tipo, int distancia) const
{
    if (tipo == Oscilatorio || distancia < 0 || distancia > 2)
        return 0;
    const qreal efectividad
        = qreal(memoria.aciertos[tipo][distancia] + 1) / (memoria.intentos[tipo][distancia] + 2);
    int coincidencias = 0;
    const int respuesta = tipo == Superior ? -1 : tipo == Inferior ? 1 : 0;
    for (int evasion : memoria.evasiones)
        if (evasion == respuesta)
            ++coincidencias;
    return efectividad + (memoria.evasiones.empty() ? 0 : 0.35 * coincidencias / memoria.evasiones.size());
}
DecisionPopeye Popeye::razonar()
{
    DecisionPopeye decision;
    int dano = 0;
    for (const auto& evento : danosRecientes)
        dano += evento.second;
    if (dano >= 3 && bloqueoRetirada <= 0)
    {
        tiempoRetirada = 4;
        bloqueoRetirada = 7;
    }
    int fallos = 0;
    for (bool acierto : memoria.ultimosAciertos)
        if (!acierto)
            ++fallos;
    tactica = tiempoRetirada > 0                             ? Retirarse
        : memoria.ultimosAciertos.size() == 5 && fallos >= 4 ? Acercarse
                                                             : Mantener;
    const qreal distancia = QLineF(objetivo, sceneBoundingRect().center()).length();
    const int grupo = distancia < 230 ? 0 : distancia < 480 ? 1 : 2;
    qreal mejor = -1;
    for (int tipo = 0; tipo < 3; ++tipo)
    {
        const qreal valor = preferenciaAtaque(TipoAtaque(tipo), grupo);
        if (valor > mejor)
        {
            mejor = valor;
            decision.ataque = TipoAtaque(tipo);
        }
    }
    if (aleatorio.bounded(100) < 20)
        decision.ataque = TipoAtaque(aleatorio.bounded(3));
    // La distancia preferida tambien procede de resultados, no de las teclas.
    qreal distanciaDeseada = 320, eficaciaMejor = 0.5;
    for (int grupoDistancia = 0; grupoDistancia < 3; ++grupoDistancia)
    {
        int intentos = 0, aciertos = 0;
        for (int tipo = 0; tipo < 3; ++tipo)
        {
            intentos += memoria.intentos[tipo][grupoDistancia];
            aciertos += memoria.aciertos[tipo][grupoDistancia];
        }
        const qreal eficacia = qreal(aciertos + 1) / (intentos + 2);
        if (intentos >= 2 && eficacia > eficaciaMejor)
        {
            eficaciaMejor = eficacia;
            distanciaDeseada = grupoDistancia == 0 ? 170 : grupoDistancia == 1 ? 350 : 600;
        }
    }
    qreal mejorDestino = 1e9;
    for (int i = 0; i < int(destinos.size()); ++i)
    {
        if (!puedeSaltarA(i))
            continue;
        const qreal nuevaDistancia = abs(destinos[i].x() - objetivo.x());
        qreal valor = abs(nuevaDistancia - distanciaDeseada);
        if (tactica == Retirarse)
            valor = -nuevaDistancia;
        else if (tactica == Acercarse)
            valor = nuevaDistancia;
        if (valor < mejorDestino)
        {
            mejorDestino = valor;
            decision.destino = i;
        }
    }
    return decision;
}
void Popeye::actuar(const DecisionPopeye& decision)
{
    if (!enSuelo || resistencia <= 0)
        return;
    if (!puedeSaltarA(decision.destino))
    {
        esperaMovimiento = 1;
        return;
    }
    const QPointF destino = destinos[decision.destino];
    velocidadSalto = QPointF((destino.x() - (x() + 22)) / duracionSalto(destino), -540);
    enSuelo = false;
    esperaMovimiento = 2;
    actualizarSprite();
}
void Popeye::actualizar(qreal dt)
{
    if (resistencia <= 0 && enSuelo)
        return;
    reloj += dt;
    poseDisparo = qMax(qreal(0), poseDisparo - dt);
    tiempoRetirada = qMax(qreal(0), tiempoRetirada - dt);
    bloqueoRetirada = qMax(qreal(0), bloqueoRetirada - dt);
    while (!danosRecientes.empty() && reloj - danosRecientes.front().first > 5)
        danosRecientes.pop_front();
    if (enSuelo)
        esperaMovimiento -= dt;
    else
    {
        const qreal piesAnteriores = y() + 64;
        moveBy(velocidadSalto.x() * dt, velocidadSalto.y() * dt + 700 * dt * dt);
        velocidadSalto.ry() += 1400 * dt;
        if (velocidadSalto.y() >= 0)
        {
            qreal altura = 1e9;
            for (const QRectF& piso : superficies)
                if (x() + 22 >= piso.left() && x() + 22 <= piso.right() && piesAnteriores <= piso.top() + 0.01
                    && y() + 64 >= piso.top())
                    altura = qMin(altura, piso.top());
            if (altura < 1e8)
            {
                setY(altura - 64);
                enSuelo = true;
                velocidadSalto = QPointF();
            }
        }
    }
    actualizarSprite();
}
void Popeye::recibirDano(int cantidad)
{
    if (resistencia <= 0 || cantidad <= 0)
        return;
    resistencia = qMax(0, resistencia - cantidad);
    danosRecientes.push_back({ reloj, cantidad });
    if (danosRecientes.size() > 32)
        danosRecientes.pop_front();
    if (enSuelo)
        esperaMovimiento = 0;
    // En vuelo recibe dano, pero conserva su velocidad y trayectoria.
    if (!resistencia)
        setOpacity(0.45);
}
void Popeye::aprender(const ExperienciaDisparo& experiencia)
{
    if (experiencia.tipo == Oscilatorio || experiencia.distancia < 0 || experiencia.distancia > 2)
        return;
    if (find(memoria.identificadores.begin(), memoria.identificadores.end(), experiencia.identificador)
        != memoria.identificadores.end())
        return;
    memoria.identificadores.push_back(experiencia.identificador);
    if (memoria.identificadores.size() > 64)
        memoria.identificadores.pop_front();
    if (experiencia.resultado == Bloqueado)
    {
        ++memoria.bloqueados;
        return;
    }
    ++memoria.experiencias;
    ++memoria.intentos[experiencia.tipo][experiencia.distancia];
    const bool acierto = experiencia.resultado == Acierto;
    if (acierto)
        ++memoria.aciertos[experiencia.tipo][experiencia.distancia];
    memoria.ultimosAciertos.push_back(acierto);
    if (memoria.ultimosAciertos.size() > 5)
        memoria.ultimosAciertos.pop_front();
    if (experiencia.observada)
    {
        memoria.evasiones.push_back(experiencia.respuesta);
        if (memoria.evasiones.size() > 5)
            memoria.evasiones.pop_front();
    }
}
void Popeye::mostrarDisparo()
{
    poseDisparo = 0.2;
    actualizarSprite();
}
void Popeye::actualizarSprite()
{
    const int pose = !enSuelo ? 3 : poseDisparo > 0 ? 2 : 0;
    mostrarFotograma(fotogramas[izquierda ? 0 : 1][pose]);
}
