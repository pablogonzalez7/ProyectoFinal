#ifndef HACIENDA_H
#define HACIENDA_H
#include <QGraphicsScene>
#include <QPixmap>
#include <vector>
#include <array>
using namespace std;
class Jugador;
class Plataforma;
class Liana;
class Hacienda : public QGraphicsScene
{
    Q_OBJECT
public:
    enum Estado { EnJuego, Recuperando, Completado };
    explicit Hacienda(QObject *padre = nullptr);
    void actualizar(qreal segundos);
    void establecerDireccion(int horizontal);
    void saltar();
    void establecerAgarre(bool activo);
    bool estaEnLiana() const;
    const array<Liana *,2> &obtenerLianas() const;
    void reiniciar();
    QString informacionActual() const;
    Jugador *obtenerJugador() const;
    const vector<Plataforma *> &obtenerPlataformas() const;
    Estado obtenerEstado() const;
    int vidasRestantes() const;
signals:
    void informacionCambiada(const QString &texto);
    void controlesLiberados();
    void rioSuperado();
    void lianasSuperadas();
protected:
    void drawBackground(QPainter *, const QRectF &) override;
private:
    void actualizarPaso(qreal dt);
    void perderVida();
    void reaparecer();
    void publicarInformacion();
    // Propiedad de QGraphicsScene; el vector solamente guarda referencias.
    vector<Plataforma *> plataformas;
    QPixmap texturaAgua;
    qreal desplazamientoAgua = 0;
    array<Liana *,2> lianas{};
    Liana *lianaActual = nullptr;
    bool buscandoAgarre = false;
    bool rioCompletado = false;
    qreal bloqueoAgarre = 0;
    QPointF desplazamientoManos() const;
    void intentarAgarrar();
    Jugador *jugador;
    Plataforma *apoyo = nullptr;
    Estado estado = EnJuego;
    int vidas = 3;
    qreal espera = 0;
    QPointF puntoSeguro;
    QString ultimoTexto;
};
#endif
