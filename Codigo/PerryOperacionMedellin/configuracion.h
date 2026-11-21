#ifndef CONFIGURACION_H
#define CONFIGURACION_H

#include <QtGlobal>

using namespace std;

// Valores de la casa que se pueden ajustar durante las pruebas de dificultad.
namespace ConfiguracionCasa {
constexpr int ancho = 1440;
constexpr int alto = 800;
constexpr int anchoVista = 960;
constexpr int altoVista = 540;
constexpr int tamanoCelda = 40;
constexpr int columnas = ancho / tamanoCelda;
constexpr int filas = (alto + tamanoCelda - 1) / tamanoCelda;
constexpr int totalCeldas = columnas * filas;
constexpr qreal rapidezJugador = 220.0;
constexpr qreal rapidezCamuflado = 130.0;
constexpr qreal duracionCamuflaje = 4.0;
constexpr qreal rapidezPatrulla = 82.0;
constexpr qreal rapidezPersecucion = 170.0;
constexpr qreal factorAlarma = 1.25;
constexpr qreal radioVision = 170.0;
constexpr qreal pausaPatrulla = 1.1;
constexpr qreal tiempoRecogerLlave = 1.2;
constexpr qreal semianguloVision = 50.0;
constexpr qreal periodoPercepcion = 0.10;
constexpr qreal duracionBusqueda = 1.5;
constexpr qreal duracionAlarma = 30.0;
constexpr qreal demoraReinicio = 1.5;
}

#endif // CONFIGURACION_H
