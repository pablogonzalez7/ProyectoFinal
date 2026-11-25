#ifndef MEMORIACOMBATE_H
#define MEMORIACOMBATE_H
#include <QPointF>
#include <array>
#include <deque>
using namespace std;
enum TipoAtaque
{
    Directo,
    Superior,
    Inferior,
    Oscilatorio
};
enum ResultadoAtaque
{
    Fallo,
    Acierto,
    Bloqueado
};
struct ExperienciaDisparo
{
    int identificador = 0;
    TipoAtaque tipo = Directo;
    int distancia = 0; // 0 cercana, 1 media, 2 lejana.
    QPointF posicionInicial;
    int respuesta = 0; // -1 subio, 0 se mantuvo, 1 bajo.
    bool observada = false;
    ResultadoAtaque resultado = Fallo;
};
struct MemoriaCombate
{
    array<array<int, 3>, 3> intentos { };
    array<array<int, 3>, 3> aciertos { };
    deque<int> evasiones; // Como maximo cinco observaciones.
    deque<bool> ultimosAciertos; // Como maximo cinco resultados validos.
    deque<int> identificadores; // Evita contabilizar dos veces un disparo.
    int bloqueados = 0;
    int experiencias = 0;
};
struct DecisionPopeye
{
    int destino = -1;
    TipoAtaque ataque = Directo;
};
#endif
