#ifndef FCNMASK_H
#define FCNMASK_H

#include <opencv2/opencv.hpp>
#include <fcntl.h>
#include <opencv2/dnn/dnn.hpp>
#include <opencv2/video/tracking.hpp>

#include <QtCore/QObject>

using namespace cv;
using namespace std;


class fcnmask : public QObject
{
    Q_OBJECT

public:
    fcnmask( int width, int height );

    /* Realiza la segmentación de personas en original_img y guarda la máscara resultante en result_mask.
     * También guarda una copia de original_img en result_img. Este proceso se realiza indefinidamente
     */
    void apply_network_FCN();

    /* img es una imagen captada por una cámara, la imagen será usada por la inteligencia artificial para segmentar,
     * también será usada para calcular el movimiento entre esta y la última imagen procesada por la ia, aplicándoselo luego
     * a la máscara dada por la ia y obteniendo así una máscara aproximada para la imagen img (que será devuelta por la función).
     */
    Mat get_mask( Mat img );

    /* Detiene la ejecución de la red neuronal en apply_network_FCN()
     */
    void stop();

    /* Devuelve el estado de la red neuronal
     */
    int get_status();

    ~fcnmask();

private:
    cv::dnn::Net network;

    Mat result_mask;
    Mat result_img;
    Mat original_img;

    int height;
    int width;

    int status; //[-1] -> Error, [0] -> No procesando, [1] -> Procesando
};

#endif // FCNMASK_H
