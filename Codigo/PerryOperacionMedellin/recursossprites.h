#ifndef RECURSOSSprites_H
#define RECURSOSSprites_H
#include <QPixmap>
#include <QRect>
#include <QSize>
#include <QString>
using namespace std;
namespace RecursosSprites {
// Se llama al construir los objetos; la cache comparte cada variante preparada.
QPixmap cargar(const QString &ruta, const QSize &tamano, const QRect &recorte = QRect());
}
#endif
