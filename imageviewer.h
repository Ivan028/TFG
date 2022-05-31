#ifndef IMAGEVIEWER_H
#define IMAGEVIEWER_H

#include <QtGui>
#include <QGLWidget>
#include <opencv2/opencv.hpp>

using namespace cv;
using namespace std;

class imageviewer : public QGLWidget
{
    Q_OBJECT
public:
    imageviewer( QWidget *parent, Mat *img );
    ~imageviewer();

    void paintEvent( QPaintEvent * );

    // Añade un cuadrado a la cola para dibujar de tamaño y coordenadas "rect" y color "color"
    void dibujar_cuadrado( QRect rect, QColor color );

    // Cambia el mat que se va a dibujar
    void set_image( Mat *img );

    // Cambia las coordenadas a partir de las cuales se va a dibujar el mat, por defecto en (0, 0)
    void set_position( int x, int y );

private:

    QImage *q_img = nullptr;
    Mat *mat_img = nullptr;

    int pos_X, pos_Y = 0;
    int width, height;
    QRect area_viewer;

    struct cuadrado
    {
        QRect rect;
        QColor color;
    };

    queue<cuadrado> cola_cuadrados;

    bool seleccionando_window = false;
    bool seleccionando_interact = false;

    QPoint start_coords_selected, end_coords_selected;

    // Convierte el Mat "src" en un QImage "dest". "linesize" es la longitud de la línea en bytes y "height" es el número de líneas
    void mat_to_qimage( Mat src, QImage *dest, int height, int linesize );

    // Cambia el "mat_img" con "img" y actualiza las dimensiones "height", "width" y las del "area_viewer"
    void set_mat( Mat *img );

signals:
    // Cuando se use el clic derecho se seleccionará un area ("rect") y se emitirá "windowSelected"
    // Cuando se cancele la selección se emitirá un "windowCancel"
    void windowSelected( QRect rect );
    void windowCancel();

    // Cuando se use el izquierdo emitirá un "win_interact_start" al inicio y un "win_interact_update" con cada movimiento.
    // En point están las coordenadas de la posición del ratón
    void win_interact_start( QPoint point );
    void win_interact_update( QPoint point );

private:
    void mousePressEvent( QMouseEvent *event );
    void mouseMoveEvent( QMouseEvent *event );
    void mouseReleaseEvent( QMouseEvent *event );
};

#endif // IMAGEVIEWER_H
