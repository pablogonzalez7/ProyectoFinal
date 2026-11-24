#include "hacienda.h"
#include "jugador.h"
#include "plataforma.h"
#include "hipopotamo.h"
#include "liana.h"
#include "configuracionhacienda.h"
#include "recursossprites.h"
#include <QPainter>
#include <QLinearGradient>
#include <QFont>
#include <cmath>
using namespace std;
using namespace ConfiguracionHacienda;

Hacienda::Hacienda(QObject *padre) : QGraphicsScene(padre), jugador(new Jugador(nullptr))
{
    texturaAgua = RecursosSprites::cargar(":/imagenes/agua_hacienda.png",QSize(384,96));
    setSceneRect(0, 0, anchoMapa, 540);
    plataformas.reserve(14);
    const auto agregar = [this](Plataforma *p) { plataformas.push_back(p); addItem(p); };
    agregar(new Plataforma(0, 420, 300));
    agregar(new Hipopotamo(400, 415, 0));
    agregar(new Hipopotamo(590, 400, 7.7));
    agregar(new Hipopotamo(780, 420, 6.9));
    agregar(new Plataforma(990, 420, 260));
    agregar(new Hipopotamo(1380, 415, 0.4));
    agregar(new Hipopotamo(1580, 400, 8.1));
    agregar(new Hipopotamo(1790, 420, 7.3));
    agregar(new Plataforma(2010, 420, 240));
    agregar(new Hipopotamo(2370, 415, 0.8));
    agregar(new Hipopotamo(2560, 420, 0.0));
    agregar(new Plataforma(2780, 420, 320));
    agregar(new Plataforma(3460, 420, 260));
    agregar(new Plataforma(4080, 420, 400));
    lianas[0] = new Liana(QPointF(3280,100), longitudLiana, -amplitudLiana);
    lianas[1] = new Liana(QPointF(3900,100), longitudLiana, amplitudLiana);
    for (auto *liana : lianas) addItem(liana);
    addItem(jugador);
    jugador->activarModoPlataformas();
    jugador->setZValue(5);
    reiniciar();
}

void Hacienda::reiniciar()
{
    vidas = vidasIniciales;
    desplazamientoAgua = 0;
    rioCompletado = false;
    for (auto *liana : lianas) liana->reiniciar();
    puntoSeguro = QPointF(100, 420 - jugador->boundingRect().height());
    for (auto *p : plataformas) p->reiniciar();
    reaparecer();
}

void Hacienda::reaparecer()
{
    estado = EnJuego;
    espera = 0;
    if (lianaActual) lianaActual->establecerOcupada(false);
    lianaActual = nullptr;
    buscandoAgarre = false;
    bloqueoAgarre = 0;
    jugador->restablecer();
    jugador->setPos(puntoSeguro);
    jugador->setVisible(true);
    apoyo = nullptr;
    for (auto *p : plataformas) {
        if (dynamic_cast<Hipopotamo *>(p)) continue;
        const QRectF r = p->superficie();
        if (jugador->x() >= r.left() && jugador->x() + jugador->boundingRect().width() <= r.right()) {
            apoyo = p;
            jugador->aterrizar(r.top());
            break;
        }
    }
    emit controlesLiberados();
    publicarInformacion();
}

void Hacienda::establecerDireccion(int horizontal)
{
    jugador->establecerDireccion(QPointF(estado == EnJuego ? qBound(-1, horizontal, 1) : 0, 0));
}
void Hacienda::saltar()
{
    if (estado != EnJuego) return;
    if (lianaActual) {
        jugador->iniciarVuelo(lianaActual->velocidadTangencial());
        lianaActual->establecerOcupada(false);
        lianaActual = nullptr;
        bloqueoAgarre = pausaReagarre;
        publicarInformacion();
        return;
    }
    // Comprueba tambien el apoyo actual: no permite saltar cuando acaba de hundirse.
    if (apoyo && apoyo->permiteApoyo()) {
        jugador->saltar();
        apoyo = nullptr;
    }
}
void Hacienda::actualizar(qreal segundos)
{
    if (!isfinite(segundos) || segundos <= 0 || estado == Completado) return;
    if (estado == Recuperando) {
        espera -= segundos;
        if (espera <= 0) {
            if (vidas == 0) reiniciar();
            else reaparecer();
        }
        return;
    }
    // Limita retrasos puntuales y subdivide para evitar atravesar las superficies.
    qreal restante = qMin(segundos, qreal(0.05));
    while (restante > 1e-9 && estado == EnJuego) {
        const qreal dt = qMin(restante, pasoMaximo);
        actualizarPaso(dt);
        restante -= dt;
    }
    desplazamientoAgua = fmod(desplazamientoAgua + 10*qMin(segundos,qreal(0.05)),
                              qreal(texturaAgua.width()));
    // Se reutiliza el temporizador del juego; solo se repinta la franja de agua.
    update(QRectF(0,alturaAgua,anchoMapa,75));
    publicarInformacion();
}

void Hacienda::actualizarPaso(qreal dt)
{
    for (auto *p : plataformas) p->actualizar(dt);
    for (auto *liana : lianas) liana->actualizar(dt);
    bloqueoAgarre = qMax(qreal(0), bloqueoAgarre - dt);
    if (lianaActual) {
        jugador->setPos(lianaActual->puntoAgarre() - desplazamientoManos());
        return;
    }
    if (apoyo && !apoyo->permiteApoyo()) {
        apoyo = nullptr;
        jugador->establecerEnSuelo(false);
    }
    const QRectF anterior = jugador->sceneBoundingRect();
    jugador->actualizar(dt);
    jugador->setX(qBound(qreal(0), jugador->x(), anchoMapa - anterior.width()));
    QRectF actual = jugador->sceneBoundingRect();
    if (apoyo) {
        const QRectF r = apoyo->superficie();
        if (actual.right() <= r.left() || actual.left() >= r.right()) {
            apoyo = nullptr;
            jugador->establecerEnSuelo(false);
        }
    }
    if (!apoyo && jugador->velocidadVerticalActual() >= 0) {
        Plataforma *aterrizaje = nullptr;
        qreal altura = 1e9;
        for (auto *p : plataformas) {
            if (!p->permiteApoyo()) continue;
            const QRectF r = p->superficie();
            if (actual.right() <= r.left() || actual.left() >= r.right()) continue;
            // Solo se aterriza descendiendo desde arriba; no se pega a los laterales.
            if (anterior.bottom() <= r.top() + 0.01 && actual.bottom() >= r.top()
                && r.top() < altura) { aterrizaje = p; altura = r.top(); }
        }
        if (aterrizaje) {
            apoyo = aterrizaje;
            jugador->aterrizar(altura);
            if (!dynamic_cast<Hipopotamo *>(apoyo))
                puntoSeguro = QPointF(apoyo->x() + 40, altura - jugador->boundingRect().height());
        }
    }
    intentarAgarrar();
    if (lianaActual) return;
    if (!rioCompletado && apoyo == plataformas[11] && jugador->x() >= 2840) {
        rioCompletado = true;
        emit rioSuperado();
    }
    if (jugador->sceneBoundingRect().bottom() > alturaAgua) { perderVida(); return; }
    if (apoyo == plataformas.back() && jugador->x() >= 4200) {
        estado = Completado;
        establecerDireccion(0);
        emit controlesLiberados();
        publicarInformacion();
        emit lianasSuperadas();
    }
}

void Hacienda::perderVida()
{
    if (estado != EnJuego) return;
    if (lianaActual) lianaActual->establecerOcupada(false);
    lianaActual = nullptr;
    buscandoAgarre = false;
    --vidas;
    estado = Recuperando;
    espera = 1.2;
    apoyo = nullptr;
    jugador->establecerDireccion(QPointF());
    jugador->setVisible(false);
    emit controlesLiberados();
    publicarInformacion();
}

QString Hacienda::informacionActual() const
{
    if (estado == Completado)
        return "Lianas superadas! Llegaste a la entrada vigilada.\nAqui termina la version actual; el combate con los guardias sigue pendiente.";
    if (estado == Recuperando)
        return vidas ? QString("Caiste al agua. Vidas: %1/3\nVolveras a la ultima orilla segura.").arg(vidas)
                     : "Sin vidas. Reiniciando desde la entrada de la hacienda...";
    const QString consejo = lianaActual
        ? "ESPACIO: sueltate cuando avances hacia la siguiente orilla. Conservas la velocidad del balanceo."
        : jugador->x() >= 2780
          ? "Manten E y salta hacia el extremo de la liana. ESPACIO: soltarte. A/D no cambia el vuelo tras soltarte."
          : "Salta sobre los hipopotamos. Amarillo (!): van a sumergirse; espera en las orillas.";
    return QString("Hacienda | Vidas: %1/3 | Avance: %2% | %3\n%4")
        .arg(vidas).arg(qBound(0,int(100*(jugador->x()-100)/4100),100))
        .arg(jugador->x() >= 2780 ? "Lianas" : "Rio").arg(consejo);
}
void Hacienda::publicarInformacion()
{
    const QString texto = informacionActual();
    if (texto != ultimoTexto) { ultimoTexto = texto; emit informacionCambiada(texto); }
}
Jugador *Hacienda::obtenerJugador() const { return jugador; }
const vector<Plataforma *> &Hacienda::obtenerPlataformas() const { return plataformas; }
Hacienda::Estado Hacienda::obtenerEstado() const { return estado; }
int Hacienda::vidasRestantes() const { return vidas; }

void Hacienda::drawBackground(QPainter *p, const QRectF &rect)
{
    QLinearGradient cielo(0, 0, 0, 465);
    cielo.setColorAt(0, QColor("#aedbd5")); cielo.setColorAt(1, QColor("#e7edc2"));
    p->fillRect(rect, cielo);
    p->setPen(Qt::NoPen);
    // Dibuja solamente la region visible, sin crear objetos nuevos por fotograma.
    const int comienzo = int(rect.left() / 170) * 170;
    for (int x = comienzo; x < rect.right() + 170; x += 170) {
        p->setBrush(QColor("#7fae8a")); p->drawEllipse(QRectF(x - 80, 220, 260, 230));
        p->setBrush(QColor("#4e806c")); p->drawEllipse(QRectF(x, 290, 180, 180));
    }
    const QRectF zonaAgua = rect.intersected(QRectF(0,alturaAgua,anchoMapa,75));
    if (!zonaAgua.isEmpty()) {
        p->drawTiledPixmap(zonaAgua,texturaAgua,
                          QPointF(zonaAgua.left()+desplazamientoAgua,zonaAgua.top()-alturaAgua));
        p->setPen(QPen(QColor("#a2e3da"),2));
        p->drawLine(QPointF(zonaAgua.left(),alturaAgua),QPointF(zonaAgua.right(),alturaAgua));
    }
    p->setPen(QPen(QColor("#4b4a2e"),14,Qt::SolidLine,Qt::RoundCap));
    for (auto *liana : lianas) {
        const QPointF anclaje = liana->pos();
        p->drawLine(anclaje + QPointF(-210,-20), anclaje + QPointF(100,-20));
        p->drawLine(anclaje + QPointF(0,-20),anclaje);
    }
    p->setFont(QFont("Arial", 12, QFont::Bold));
    p->setPen(QColor("#244e49"));
    p->drawText(QRectF(40, 280, 245, 85), Qt::AlignCenter, "ENTRADA A LA HACIENDA\nA/D + ESPACIO");
    p->drawText(QRectF(990, 300, 260, 70), Qt::AlignCenter, "ORILLA SEGURA");
    p->drawText(QRectF(2010, 300, 240, 70), Qt::AlignCenter, "ORILLA SEGURA");
    p->drawText(QRectF(3460, 285, 260, 85), Qt::AlignCenter, "ORILLA SEGURA\nOtra liana adelante");
    p->drawText(QRectF(4110, 285, 320, 85), Qt::AlignCenter, "ENTRADA VIGILADA");
    p->drawText(QRectF(2790, 285, 290, 85), Qt::AlignCenter, "LIANAS\nManten E + salta");
}

QPointF Hacienda::desplazamientoManos() const
{
    return QPointF(jugador->boundingRect().width()/2, 8);
}
void Hacienda::establecerAgarre(bool activo)
{
    buscandoAgarre = activo && estado == EnJuego;
}
bool Hacienda::estaEnLiana() const { return lianaActual != nullptr; }
const array<Liana *,2> &Hacienda::obtenerLianas() const { return lianas; }
void Hacienda::intentarAgarrar()
{
    if (!buscandoAgarre || bloqueoAgarre > 0 || jugador->estaEnSuelo()) return;
    const QPointF manos = jugador->pos() + desplazamientoManos();
    for (auto *liana : lianas) {
        const QPointF diferencia = liana->puntoAgarre() - manos;
        if (QPointF::dotProduct(diferencia,diferencia) > radioAgarre*radioAgarre) continue;
        apoyo = nullptr;
        jugador->iniciarVuelo(QPointF());
        lianaActual = liana;
        liana->establecerOcupada(true);
        jugador->setPos(liana->puntoAgarre() - desplazamientoManos());
        return;
    }
}
