#ifndef MENUANIMADO_H
#define MENUANIMADO_H
#include <QWidget>
#include <QPixmap>
#include <QTimer>
#include <array>
using namespace std;

// Decoracion del menu: no crea personajes ni ejecuta fisicas del juego.
class MenuAnimado : public QWidget
{
    Q_OBJECT
public:
    explicit MenuAnimado(QWidget* padre = nullptr);
protected:
    void paintEvent(QPaintEvent*) override;
    void showEvent(QShowEvent*) override;
    void hideEvent(QHideEvent*) override;
private slots:
    void animar();
private:
    void prepararFondo();
    void prepararTitulo();
    QPixmap fondo, titulo, bala;
    array<QPixmap, 4> perry, popeye, escobar, hipopotamo;
    array<QPixmap, 2> policia;
    QTimer* temporizador;
    int paso = 0;
};
#endif
