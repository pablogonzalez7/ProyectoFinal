#ifndef EVENTOSSONIDO_H
#define EVENTOSSONIDO_H
// Identificadores para comunicar escenas y sonido mediante senales Qt.
namespace Sonido
{
enum Evento
{
    Llave,
    Alarma,
    Disparo,
    Impacto,
    Escudo,
    Victoria,
    Derrota,
    Salto,
    Camuflaje,
    Total
};
enum Musica
{
    Sigilo,
    Accion,
    Perry,
    Popeye,
    VictoriaPopeye,
    TotalPistas
};
}
#endif
