#ifndef CONFIGURACIONHACIENDA_H
#define CONFIGURACIONHACIENDA_H
#include <QtGlobal>
using namespace std;
namespace ConfiguracionHacienda {
// Un metro corresponde a 100 pixeles. Valores ajustados para el salto jugable.
constexpr qreal pixelesPorMetro = 100.0;
constexpr qreal rapidezJugador = 250.0; // px/s = 2.5 m/s
constexpr qreal gravedad = 1400.0; // px/s^2 = 14 m/s^2
constexpr qreal impulsoSalto = 540.0; // px/s = 5.4 m/s
constexpr qreal pasoMaximo = 1.0 / 120.0;
constexpr qreal anchoMapa = 4480.0;
constexpr qreal alturaAgua = 465.0;
constexpr qreal radioAgarre = 45.0;
constexpr qreal pausaReagarre = 0.5;
constexpr qreal longitudLiana = 260.0;
constexpr qreal amplitudLiana = 0.85; // radianes
constexpr int vidasIniciales = 3;
constexpr qreal periodoHipopotamo = 8.5;
constexpr qreal inicioAviso = 4.5;
constexpr qreal inicioInmersion = 5.5;
constexpr qreal inicioSalida = 8.0;
}
#endif
