#ifndef DIFICULTAD_H
#define DIFICULTAD_H
#include "configuracion.h"
using namespace std;
enum class Dificultad
{
    Facil,
    Normal,
    Dificil
};
struct PerfilGuardia
{
    qreal radio = ConfiguracionCasa::radioVision;
    qreal semiangulo = ConfiguracionCasa::semianguloVision;
    bool investiga = true;
    bool comparteAvisos = false;
    bool revisaSectores = false;
};
inline PerfilGuardia perfilGuardia(Dificultad dificultad)
{
    PerfilGuardia perfil;
    if (dificultad == Dificultad::Facil)
    {
        perfil.radio = 150;
        perfil.semiangulo = 40;
        perfil.investiga = false;
    }
    else if (dificultad == Dificultad::Dificil)
    {
        perfil.radio = 200;
        perfil.semiangulo = 60;
        perfil.comparteAvisos = true;
        perfil.revisaSectores = true;
    }
    return perfil;
}
inline const char* nombreDificultad(Dificultad dificultad)
{
    if (dificultad == Dificultad::Facil)
        return "Facil";
    if (dificultad == Dificultad::Dificil)
        return "Dificil";
    return "Normal";
}
#endif
