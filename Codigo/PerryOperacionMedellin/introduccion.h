#ifndef INTRODUCCION_H
#define INTRODUCCION_H
#include <QWidget>
#include <QPixmap>
#include <QElapsedTimer>
#include <array>
using namespace std;
class QTimer;
class QMediaPlayer;
class QAudioOutput;
class QPainter;

// Cinematica dibujada en Qt. Los sprites se cargan una vez y se reutilizan.
class Introduccion : public QWidget
{
    Q_OBJECT
public:
    explicit Introduccion(QWidget *padre = nullptr);
    bool estaActiva() const { return activa; }
    qreal tiempoActual() const { return segundos; }
    void avanzar(qreal dt);
public slots:
    void iniciar();
    void terminar();
signals:
    void terminada();
protected:
    void paintEvent(QPaintEvent *) override;
    void keyPressEvent(QKeyEvent *) override;
    void keyReleaseEvent(QKeyEvent *) override;
    bool eventFilter(QObject *, QEvent *) override;
private slots:
    void animar();
    void errorAudio();
private:
    void dibujarLaboratorio(QPainter &p);
    void dibujarHacienda(QPainter &p);
    void dibujarPortal(QPainter &p, QPointF centro, qreal escala);
    void dibujarActor(QPainter &p, const QPixmap &sprite, QPointF pies,
                      qreal altura, qreal giro = 0, qreal escala = 1, bool espejo = false);
    void dibujarSubtitulo(QPainter &p);
    QPixmap laboratorio, hacienda, doctor, doctorAccion;
    array<QPixmap,4> perry, popeye, escobar;
    array<QPixmap,2> policias;
    QTimer *timer;
    QMediaPlayer *musica;
    QAudioOutput *salida;
    QElapsedTimer reloj;
    qreal segundos = 0;
    bool activa = false, pausada = false, silenciada = false;
    int teclaSalto = 0;
};
#endif
