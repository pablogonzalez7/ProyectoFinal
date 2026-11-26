#include "nivel.h"
#include "jugador.h"
#include "hipopotamo.h"
#include "hacienda.h"
#include "configuracionhacienda.h"
#include <QApplication>
#include <cmath>
#include <iostream>
#include <stdexcept>
using namespace std;
void exigir(bool ok, const char *mensaje) { if (!ok) throw runtime_error(mensaje); }
int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    try {
        Nivel casa;
        Jugador *perry = casa.obtenerJugador();
        exigir(perry->camuflajesRestantes() == 2, "No inicia con dos usos");
        perry->alternarCamuflaje();
        exigir(perry->estaCamuflado() && perry->camuflajesRestantes() == 1, "Primer uso incorrecto");
        perry->actualizarCamuflaje(1);
        perry->alternarCamuflaje();
        exigir(!perry->estaCamuflado() && perry->camuflajesRestantes() == 1, "Cancelar consume o devuelve un uso");
        perry->alternarCamuflaje();
        exigir(perry->estaCamuflado() && perry->camuflajesRestantes() == 0, "Segundo uso incorrecto");
        perry->actualizarCamuflaje(3.9);
        exigir(perry->estaCamuflado(), "Dura menos de cuatro segundos");
        perry->actualizarCamuflaje(0.2);
        exigir(!perry->estaCamuflado(), "No termina el segundo camuflaje");
        for (int i=0; i<10; ++i) perry->alternarCamuflaje();
        exigir(!perry->estaCamuflado() && perry->camuflajesRestantes() == 0, "Permite un tercer uso");
        casa.reiniciarCasa();
        exigir(perry->camuflajesRestantes() == 2 && !perry->estaCamuflado(), "No repone al reiniciar casa");
        casa.actualizar(0.001);
        exigir(casa.informacionActual().contains("Camuflajes: 2/2"), "Contador no se actualiza");
        cout << "OK: dos usos, cancelacion, duracion, bloqueo y reinicio" << endl;
        Hipopotamo hip(0,0,0);
        hip.actualizar(4.5);
        exigir(hip.estaAvisando() && hip.permiteApoyo(), "Falta aviso");
        hip.actualizar(0.99);
        exigir(hip.estaAvisando() && hip.permiteApoyo(), "Aviso demasiado corto");
        hip.actualizar(0.02);
        exigir(!hip.permiteApoyo(), "Apoyo bajo el agua");
        Hacienda rio;
        const int objetos=rio.items().size();
        qreal faseAnterior=-1;
        int cambios=0;
        for (int intento=0; intento<100; ++intento) {
            rio.reiniciar();
            int cantidad=0;
            for (auto *p : rio.obtenerPlataformas()) {
                auto *h=dynamic_cast<Hipopotamo *>(p);
                if (!h) continue;
                const qreal fase=h->faseActual();
                exigir(fase>=0 && fase<ConfiguracionHacienda::periodoHipopotamo, "Desfase fuera del ciclo");
                if (cantidad==0) { if (fase!=faseAnterior) ++cambios; faseAnterior=fase; }
                ++cantidad;
            }
            exigir(cantidad==8 && rio.items().size()==objetos, "Cambia cantidad de objetos al reiniciar");
        }
        exigir(cambios>1, "Repite siempre el mismo desfase");
        cout << "OK: fases aleatorias en 100 reinicios, aviso y apoyo, objetos reutilizados" << endl;
    } catch (const exception &e) { cerr << e.what() << endl; return 1; }
    return 0;
}
