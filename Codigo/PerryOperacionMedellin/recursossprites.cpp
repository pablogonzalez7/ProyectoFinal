#include "recursossprites.h"
#include <QPixmapCache>
#include <QImage>
#include <stdexcept>
using namespace std;
QPixmap RecursosSprites::cargar(const QString &ruta, const QSize &tamano, const QRect &recorte)
{
    const QString clave = QString("juego_sprite_v1:%1:%2,%3:%4,%5,%6,%7")
        .arg(ruta).arg(tamano.width()).arg(tamano.height())
        .arg(recorte.x()).arg(recorte.y()).arg(recorte.width()).arg(recorte.height());
    QPixmap resultado;
    if (QPixmapCache::find(clave, &resultado)) return resultado;
    QImage imagen(ruta);
    if (imagen.isNull()) throw runtime_error(("No se pudo cargar el sprite " + ruta).toStdString());
    if (recorte.isValid()) imagen = imagen.copy(recorte);
    imagen = imagen.convertToFormat(QImage::Format_ARGB32);
    // Busca una sola vez el contenido visible para quitar los margenes vacios.
    int izquierda = imagen.width(), derecha = -1, arriba = imagen.height(), abajo = -1;
    for (int y = 0; y < imagen.height(); ++y) {
        const auto *fila = reinterpret_cast<const QRgb *>(imagen.constScanLine(y));
        for (int x = 0; x < imagen.width(); ++x) {
            if (qAlpha(fila[x]) < 192) continue;
            izquierda = qMin(izquierda, x); derecha = qMax(derecha, x);
            arriba = qMin(arriba, y); abajo = qMax(abajo, y);
        }
    }
    if (derecha < izquierda) throw runtime_error("El sprite no contiene pixeles visibles.");
    resultado = QPixmap::fromImage(imagen.copy(QRect(QPoint(izquierda, arriba), QPoint(derecha, abajo))))
        .scaled(tamano, Qt::IgnoreAspectRatio, Qt::FastTransformation);
    QPixmapCache::insert(clave, resultado);
    return resultado;
}
