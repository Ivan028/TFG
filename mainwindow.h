#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <stdio.h>
#include <QTimer>
#include <opencv2/opencv.hpp>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>
#include <experimental/filesystem>
#include <thread>

#include <opencv2/objdetect.hpp>

#include <imgviewer.h>

using namespace cv;
using namespace std;

QT_BEGIN_NAMESPACE

namespace Ui {
    class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    QTimer timer;
    VideoCapture *cam;
    int output;
    size_t framesize;
    ImgViewer *visorS, *visorD;
    Mat origenImagen, camImagen, destinoImagen, procesadaImagen;
    bool winSelected;
    Rect imageWindow;

    Mat grabacion[1800];
    int grabacion_duracion;
    int reproducion_posicion;
    int framerate_grabacion;

    Mat escala_grises(Mat img);
    Mat recortar_video(Mat img);
    Mat recortar_alargar_video(Mat img);
    Mat ecualizado(Mat img);
    Mat filtro_mediana(Mat img);
    Mat controlRGB(Mat img);

    void grabar(Mat img);

    void procesar_Video();

public slots:
    void compute();
    void selectWindow(QPointF p, int w, int h);
    void deselectWindow();

    void cambiar_framerate(int value);
    void pausar_Button(bool enable);

    void escala_grises_Button(bool enable);
    void zoom_Button(bool enable);
    void grabar_Button(bool enable);
    void reproducir_Button(bool enable);
    void recortar_Button(bool enable);

};
#endif // MAINWINDOW_H
