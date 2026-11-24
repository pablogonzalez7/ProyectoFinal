#ifndef CONFIGURACIONCARRETERA_H
#define CONFIGURACIONCARRETERA_H
#include <QtGlobal>
using namespace std;
namespace ConfiguracionCarretera {
constexpr qreal pixelesPorMetro=10.0;
constexpr qreal masa=1000.0; // kg
constexpr qreal gravedad=9.81;
constexpr qreal fuerzaMotor=6000.0; // N: modelo simplificado de traccion constante
constexpr qreal rozamientoSeco=0.02;
constexpr qreal rozamientoLodo=1.5; // Resistencia efectiva ajustada para el juego
constexpr qreal velocidadMaxima=28.0; // m/s
constexpr qreal velocidadMinima=6.0; // Limite jugable para no quedar inmovil en lodo
constexpr qreal longitudRecorrido=1200.0;
constexpr qreal velocidadPerseguidor=24.0;
constexpr qreal separacionInicial=35.0;
constexpr qreal separacionMaxima=60.0;
constexpr qreal duracionEscudo=1.0;
constexpr qreal recargaEscudo=7.0; // Desde activacion: 1 s activo y 6 s de espera.
constexpr qreal intervaloDisparo=2.0;
constexpr qreal avisoDisparo=0.55;
constexpr int cantidadProyectiles=12;
constexpr int cantidadObstaculos=6;
constexpr qreal separacion=340.0;
}
#endif
