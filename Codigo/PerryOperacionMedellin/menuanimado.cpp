#include "menuanimado.h"
#include "recursossprites.h"
#include <QPainter>
#include <QPainterPath>
#include <QLinearGradient>
#include <QShowEvent>
#include <QHideEvent>
#include <QtMath>
using namespace std;

MenuAnimado::MenuAnimado(QWidget* padre) : QWidget(padre)
{
    setObjectName("menuModos");
    prepararFondo();
    prepararTitulo();
    for (int i = 0; i < 4; ++i)
    {
        const int columnas[] = {190, 470, 742, 1015};
        perry[i] = RecursosSprites::cargar(":/imagenes/perry.png", QSize(174, 120),
            QRect(columnas[i], 558, 208, 144));
        popeye[i] = RecursosSprites::cargar(":/imagenes/popeye.png", QSize(86, 125),
            QRect(i * 543, 0, 543, 724));
        escobar[i] = RecursosSprites::cargar(":/imagenes/escobar.png", QSize(78, 119),
            QRect(i * 543, 0, 543, 724));
        hipopotamo[i] = RecursosSprites::cargar(":/imagenes/hipopotamos.png", QSize(177, 94),
            QRect((i % 2) * 627, (i / 2) * 627 + 195, 627, 382));
    }
    for (int i = 0; i < 2; ++i)
        policia[i] = RecursosSprites::cargar(":/imagenes/policias.png", QSize(55, 85),
            QRect(97, i * 443 + 17, 248, 407));
    bala = RecursosSprites::cargar(":/imagenes/bala.png", QSize(10, 24));
    temporizador = new QTimer(this);
    temporizador->setObjectName("animacionMenu");
    temporizador->setInterval(80); // 12.5 repintados/s son suficientes para esta decoracion.
    connect(temporizador, SIGNAL(timeout()), this, SLOT(animar()));
}

void MenuAnimado::prepararFondo()
{
    // Composicion estatica calculada una vez. Se reutilizan los PNG del juego.
    fondo = QPixmap(960, 660);
    fondo.fill(QColor("#112c35"));
    QPainter p(&fondo);
    p.drawPixmap(0, 0, RecursosSprites::cargar(":/imagenes/panorama_hacienda.png", QSize(960, 320)));
    p.drawPixmap(0, 320, RecursosSprites::cargar(":/imagenes/llano_carretera.png", QSize(960, 340)));
    p.fillRect(fondo.rect(), QColor(4, 26, 39, 64));

    // Casa en miniatura: las seis habitaciones y los nueve muebles de su atlas.
    p.setPen(QPen(QColor("#f8c85d"), 3));
    p.setBrush(QColor("#203c3e"));
    p.drawRoundedRect(QRectF(18, 198, 219, 215), 10, 10);
    p.drawPixmap(25, 205, RecursosSprites::cargar(":/imagenes/pisos_casa.png", QSize(205, 198)));
    for (int i = 0; i < 9; ++i)
    {
        QPixmap mueble = RecursosSprites::cargar(":/imagenes/muebles_casa.png", QSize(44, 43),
            QRect((i % 3) * 418, (i / 3) * 418, 418, 418));
        p.drawPixmap(38 + (i % 3) * 65, 222 + (i / 3) * 61, mueble);
    }
    // Interior de la hacienda, detras de Popeye y Escobar.
    p.drawRoundedRect(QRectF(722, 198, 219, 215), 10, 10);
    p.drawPixmap(728, 205, RecursosSprites::cargar(":/imagenes/interior_hacienda.png", QSize(207, 201)));
    p.fillRect(QRect(728, 205, 207, 201), QColor(5, 22, 30, 46));

    // Carretera, vehiculos y obstaculos.
    p.drawPixmap(28, 429, RecursosSprites::cargar(":/imagenes/asfalto_carretera.png", QSize(188, 231)));
    p.drawPixmap(49, 481, RecursosSprites::cargar(":/imagenes/carro_perry.png", QSize(49, 86)));
    p.drawPixmap(130, 554, RecursosSprites::cargar(":/imagenes/carro_policia.png", QSize(48, 84)));
    p.drawPixmap(123, 510, RecursosSprites::cargar(":/imagenes/barrera.png", QSize(64, 38)));
    p.drawPixmap(46, 602, RecursosSprites::cargar(":/imagenes/lodo.png", QSize(55, 39)));

    p.drawPixmap(228, 560, RecursosSprites::cargar(":/imagenes/agua_hacienda.png", QSize(732, 100)));
    p.drawPixmap(215, 624, RecursosSprites::cargar(":/imagenes/suelo_hacienda.png", QSize(745, 36)));
    p.drawPixmap(3, 0, RecursosSprites::cargar(":/imagenes/liana.png", QSize(26, 192)));
    p.drawPixmap(925, 0, RecursosSprites::cargar(":/imagenes/liana.png", QSize(27, 190)));
    // Panel opaco bajo los controles: los sprites nunca dificultan leer las opciones.
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(QPen(QColor(104, 239, 207, 210), 2));
    p.setBrush(QColor(9, 29, 44, 244));
    p.drawRoundedRect(QRectF(248, 203, 464, 357), 24, 24);
}

void MenuAnimado::prepararTitulo()
{
    titulo = QPixmap(600, 182);
    titulo.fill(Qt::transparent);
    QPainter p(&titulo);
    p.setRenderHint(QPainter::Antialiasing);
    QFont letra("Impact", 78, QFont::Black);
    letra.setStyleHint(QFont::SansSerif);
    letra.setItalic(true);
    letra.setPixelSize(106);
    QPainterPath texto;
    texto.addText(0, 110, letra, "PERRY");
    QRectF limites = texto.boundingRect();
    p.translate(300 - limites.center().x(), 0);
    p.translate(5, 9);
    p.fillPath(texto, QColor("#072637"));
    p.translate(-5, -9);
    QLinearGradient color(0, 30, 0, 115);
    color.setColorAt(0, QColor("#fff48a"));
    color.setColorAt(0.5, QColor("#ffcc48"));
    color.setColorAt(1, QColor("#ff873d"));
    p.setPen(QPen(QColor("#072637"), 9, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.setBrush(color);
    p.drawPath(texto);
    p.resetTransform();
    p.setFont(QFont("Arial", 19, QFont::Black));
    p.setPen(QColor("#082a38"));
    p.drawText(QRect(2, 134, 600, 40), Qt::AlignCenter, "OPERACIÓN MEDELLÍN");
    p.setPen(QColor("#edfff4"));
    p.drawText(QRect(0, 131, 600, 40), Qt::AlignCenter, "OPERACIÓN MEDELLÍN");
}

void MenuAnimado::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.scale(width() / 960.0, height() / 660.0);
    p.drawPixmap(0, 0, fondo);
    const int fotograma = (paso / 3) % 4;
    const qreal balanceo = qSin(qDegreesToRadians(paso * 3.6)) * 3;
    p.drawPixmap(QPointF(180, 9 + balanceo), titulo);
    p.drawPixmap(QPointF(43, 374 + balanceo), perry[fotograma]);
    p.drawPixmap(183, 427, policia[(paso / 4) % 2]);
    p.drawPixmap(744, 287, popeye[fotograma]);
    p.drawPixmap(847, 290, escobar[fotograma]);
    p.drawPixmap(QPointF(747, 527 + balanceo), hipopotamo[fotograma]);
    // Franja de agua por delante: el hipopotamo se ve semisumergido.
    p.fillRect(QRectF(744, 603 + balanceo, 182, 15), QColor(33, 148, 160, 165));
    p.drawPixmap(108, 534 - (paso * 3) % 28, bala);
    // Destellos discretos junto al titulo, sin crear items en cada fotograma.
    p.setPen(QPen(QColor("#ffec82"), 3));
    for (QPointF centro : { QPointF(245, 75 + balanceo), QPointF(716, 113 - balanceo) })
    {
        p.drawLine(centro + QPointF(-7, 0), centro + QPointF(7, 0));
        p.drawLine(centro + QPointF(0, -7), centro + QPointF(0, 7));
    }
}
void MenuAnimado::animar()
{
    paso = (paso + 1) % 600;
    update();
}
void MenuAnimado::showEvent(QShowEvent* evento)
{
    QWidget::showEvent(evento);
    temporizador->start();
}
void MenuAnimado::hideEvent(QHideEvent* evento)
{
    temporizador->stop();
    QWidget::hideEvent(evento);
}
