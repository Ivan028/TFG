#include "mainwindow.h"
#include "ui_mainwindow.h"

#define VID_WIDTH  640
#define VID_HEIGHT 480
#define VIDEO_IN   "/dev/video0"
#define VIDEO_OUT  "/dev/video5"
#define FOTOGRAMAS_GRABACION  1800

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    winSelected = false;

    // Abrimos la cámara
    cam = new VideoCapture(VIDEO_IN);

    if (not cam->isOpened()) {
        std::cerr << "ERROR: no se ha podido abrir la cámara\n";
        return;
    }

    cam->set(cv::CAP_PROP_FRAME_WIDTH, VID_WIDTH);
    cam->set(cv::CAP_PROP_FRAME_HEIGHT, VID_HEIGHT);

    // Abrimos la salida
    output = open(VIDEO_OUT, O_WRONLY);
    if(output < 0) {
        std::cerr << "ERROR: No se ha podido abrir el dispositivo de salida: " << strerror(errno) << "\n";
        return;
    }

    // Obtenemos el formato del dispositivo
    struct v4l2_format vid_format;
    memset(&vid_format, 0, sizeof(vid_format));
    vid_format.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;

    if (ioctl(output, VIDIOC_G_FMT, &vid_format) < 0) {
        std::cerr << "ERROR: No se ha podido obtener el formato de video: " << strerror(errno) << "\n";
        return;
    }

    // Configuramos el formato del video de salida
    framesize = VID_WIDTH * VID_HEIGHT * 1.5;
    vid_format.fmt.pix.width = cam->get(cv::CAP_PROP_FRAME_WIDTH);
    vid_format.fmt.pix.height = cam->get(cv::CAP_PROP_FRAME_HEIGHT);
    vid_format.fmt.pix.pixelformat = V4L2_PIX_FMT_YUV420;
    vid_format.fmt.pix.sizeimage = framesize;
    vid_format.fmt.pix.field = V4L2_FIELD_NONE;

    if (ioctl(output, VIDIOC_S_FMT, &vid_format) < 0) {
        std::cerr << "ERROR: No se ha podido establecer el formato: " << strerror(errno) << "\n";
        return;
    }

    origenImagen.create(240, 320, CV_8UC3);
    camImagen.create(VID_WIDTH, VID_HEIGHT, CV_8UC3);
    destinoImagen.create(240, 320, CV_8UC3);
    procesadaImagen.create(VID_WIDTH, VID_HEIGHT, CV_8UC3);

    //Visores
    visorS = new ImgViewer(&origenImagen, ui->imageFrameS);
    visorD = new ImgViewer(&destinoImagen, ui->imageFrameD);

    //Conexiones con los slots
    connect(&timer, SIGNAL(timeout()), this, SLOT(compute()));
    connect(ui->framerate_box, SIGNAL(valueChanged(int)), this, SLOT(cambiar_framerate(int)));
    connect(ui->pausar_bt,SIGNAL(clicked(bool)),this,SLOT(pausar_Button(bool)));
    connect(ui->escala_grises_bt,SIGNAL(clicked(bool)),this,SLOT(escala_grises_Button(bool)));
    connect(ui->grabar_bt,SIGNAL(clicked(bool)),this,SLOT(grabar_Button(bool)));
    connect(ui->reproducir_bt,SIGNAL(clicked(bool)),this,SLOT(reproducir_Button(bool)));
    connect(visorS,SIGNAL(windowSelected(QPointF, int, int)),this,SLOT(selectWindow(QPointF, int, int)));
    //connect(visorS,SIGNAL(pressEvent()),this,SLOT(deselectWindow()));
    connect(ui->zoom_bt,SIGNAL(clicked(bool)),this,SLOT(zoom_Button(bool)));
    connect(ui->recortar_bt,SIGNAL(clicked(bool)),this,SLOT(recortar_Button(bool)));

    timer.setTimerType(Qt::PreciseTimer);
    timer.start(1.0/ui->framerate_box->value()*1000);

    grabacion_duracion = 0;
}


// ----- OPERACIONES -----

void MainWindow::cambiar_framerate(int value) {
    timer.start(1.0/value*1000);
}

Mat MainWindow::escala_grises(Mat img) {
    Mat aux;
    cv::cvtColor(img, aux, cv::COLOR_RGB2GRAY);
    cv::cvtColor(aux, aux, cv::COLOR_GRAY2RGB);
    return aux;
}

Mat MainWindow::recortar_video(Mat img) {
    Mat aux;
    aux.create(VID_HEIGHT, VID_WIDTH, CV_8UC3);
    aux.setTo(Scalar::all(0));

    Rect imageWindowAux = imageWindow;
    imageWindowAux.x = imageWindowAux.x*(VID_WIDTH/320.0);
    imageWindowAux.y = imageWindowAux.y*(VID_HEIGHT/240.0);
    imageWindowAux.height = imageWindowAux.height*(VID_HEIGHT/240.0);
    imageWindowAux.width = imageWindowAux.width*(VID_WIDTH/320.0);
    Mat aux2 = Mat(img, imageWindowAux);

    //Calculamos la posicion para centrar la imagen
    imageWindowAux.x = int((VID_WIDTH-imageWindowAux.width)/2);
    imageWindowAux.y = int((VID_HEIGHT-imageWindowAux.height)/2);

    //Copiamos la region de interes en el centro de los destinos
    aux2.copyTo(aux(imageWindowAux));
    return aux;
}

Mat MainWindow::recortar_alargar_video(Mat img) {
    Mat aux;
    aux.create(VID_HEIGHT, VID_WIDTH, CV_8UC3);
    aux.setTo(Scalar::all(0));

    Rect imageWindowAux = imageWindow;
    imageWindowAux.x = imageWindowAux.x*(VID_WIDTH/320.0);
    imageWindowAux.y = imageWindowAux.y*(VID_HEIGHT/240.0);
    imageWindowAux.height = imageWindowAux.height*(VID_HEIGHT/240.0);
    imageWindowAux.width = imageWindowAux.width*(VID_WIDTH/320.0);
    Mat aux2 = Mat(img, imageWindowAux);

    //Calculamos la posicion para centrar la imagen
    float escala;
    //si el ancho es el mayor ampliamos todo el ancho
    if(((float)imageWindowAux.width/VID_WIDTH) > ((float)imageWindowAux.height/VID_HEIGHT)) {
        escala = float ((float)VID_WIDTH / imageWindowAux.width);
        imageWindowAux.width = VID_WIDTH;
        imageWindowAux.height = int(float(imageWindowAux.height)*escala);
    //Si el alto es el mayor ampliamos todo el alto
    } else {
        escala = float((float)VID_HEIGHT / imageWindowAux.height);
        imageWindowAux.width = int(float(imageWindowAux.width)*escala);
        imageWindowAux.height = VID_HEIGHT;
    }
    cv::resize(aux2, aux2, Size(imageWindowAux.width, imageWindowAux.height));
    imageWindowAux.x = int((VID_WIDTH-imageWindowAux.width)/2);
    imageWindowAux.y = int((VID_HEIGHT-imageWindowAux.height)/2);

    //Copiamos la region de interes en el centro de los destinos
    aux2.copyTo(aux(imageWindowAux));
    return aux;
}

Mat MainWindow::ecualizado(Mat img) {
    Mat aux;
    equalizeHist(img, aux);
    return aux;
}

Mat MainWindow::filtro_mediana(Mat img) {
    Mat aux;
    medianBlur(img, aux, 5);
    return aux;
}

Mat MainWindow::controlRGB(Mat img) {
    Mat aux, channels[3];
    split(img, channels);
    channels[0] = channels[0] * (float(ui->color_barra_R->value())/100);
    channels[1] = channels[1] * (float(ui->color_barra_G->value())/100);
    channels[2] = channels[2] * (float(ui->color_barra_B->value())/100);
    merge(channels, 3, aux);
    return aux;
}

void MainWindow::grabar(Mat img) {
    if(grabacion_duracion < FOTOGRAMAS_GRABACION) {
        grabacion[grabacion_duracion] = img;
        grabacion_duracion++;
        ui->duracion_txt->setText("Quedan "+QString::number(ceil((float)(FOTOGRAMAS_GRABACION-grabacion_duracion)/framerate_grabacion))+" s");
        ui->progreso_barra->setValue(100-(100*(float)grabacion_duracion/FOTOGRAMAS_GRABACION));
    } else {
        ui->grabar_bt->setText("Grabar");
        ui->duracion_txt->setText("");
        ui->grabar_bt->setChecked(false);
        ui->framerate_box->setEnabled(true);
    }
}


// ----- BOTONES -----

void MainWindow::escala_grises_Button(bool enable) {
    if (enable) {
        ui->escala_grises_bt->setText("Color");
    } else {
        ui->escala_grises_bt->setText("Escala de grises");
    }
}

void MainWindow::pausar_Button(bool enable) {
    if (enable) {
        ui->pausar_bt->setText("Activar Imagen");
    } else {
        ui->pausar_bt->setText("Pausar Imagen");
    }
}

void MainWindow::zoom_Button(bool enable) {
    if (enable) {
        if(winSelected) {
            ui->zoom_bt->setText("Quitar zoom");
            recortar_Button(false);
            ui->recortar_bt->setChecked(false);
        } else {
            ui->zoom_bt->setChecked(false);
        }
    } else {
        ui->zoom_bt->setText("Zoom");
    }
}

void MainWindow::recortar_Button(bool enable) {
    if (enable) {
        if(winSelected) {
            ui->recortar_bt->setText("Deshacer recorte");
            zoom_Button(false);
            ui->zoom_bt->setChecked(false);
        } else {
            ui->recortar_bt->setChecked(false);
        }
    } else {
        ui->recortar_bt->setText("Recorte");
    }
}

void MainWindow::grabar_Button(bool enable) {
    if (enable) {
        ui->grabar_bt->setText("Detener");
        ui->reproducir_bt->setChecked(false);
        ui->reproducir_bt->setText("Reproducir");
        grabacion_duracion = 0;
        ui->progreso_barra->setInvertedAppearance(true);
        ui->framerate_box->setEnabled(false);
        framerate_grabacion = ui->framerate_box->value();
    } else {
        ui->grabar_bt->setText("Grabar");
        ui->duracion_txt->setText("");
        ui->framerate_box->setEnabled(true);
    }
}

void MainWindow::reproducir_Button(bool enable) {
    if (enable && grabacion_duracion > 0) {
        ui->reproducir_bt->setText("Detener");
        ui->grabar_bt->setChecked(false);
        ui->grabar_bt->setText("Grabar");
        reproducion_posicion = 0;
        ui->progreso_barra->setInvertedAppearance(false);
        ui->framerate_box->setEnabled(false);
        cambiar_framerate(framerate_grabacion);
    } else {
        ui->reproducir_bt->setChecked(false);
        ui->reproducir_bt->setText("Reproducir");
        ui->duracion_txt->setText("");
        ui->framerate_box->setEnabled(true);
        cambiar_framerate(ui->framerate_box->value());
    }
}


// ----- PROCESAMIENTO -----

void MainWindow::procesar_Video() {
    if(ui->reproducir_bt->isChecked()) {
        procesadaImagen = grabacion[reproducion_posicion];
        reproducion_posicion++;
        ui->duracion_txt->setText(QString::number(ceil((float)(grabacion_duracion-reproducion_posicion)/framerate_grabacion))+"s");
        ui->progreso_barra->setValue((100*reproducion_posicion/grabacion_duracion));
        if(reproducion_posicion >= grabacion_duracion) {
            ui->reproducir_bt->setText("Reproducir");
            ui->reproducir_bt->setChecked(false);
            ui->duracion_txt->setText("");
            ui->framerate_box->setEnabled(true);
            cambiar_framerate(ui->framerate_box->value());
        }
    } else {
        procesadaImagen = camImagen.clone();

        Mat channelsYUV[3];
        cv::cvtColor(procesadaImagen, procesadaImagen, cv::COLOR_RGB2YUV);
        split(procesadaImagen, channelsYUV);

        if(ui->filtro_mediana_bt->isChecked()) {
            channelsYUV[0] = filtro_mediana(channelsYUV[0]);
        }
        if(ui->ecualizado_bt->isChecked()) {
            channelsYUV[0] = ecualizado(channelsYUV[0]);
        }

        merge(channelsYUV, 3, procesadaImagen);
        cvtColor(procesadaImagen, procesadaImagen, COLOR_YUV2RGB);

        if(ui->control_color_bt->isChecked()) {
            procesadaImagen = controlRGB(procesadaImagen);
        }

        if(ui->escala_grises_bt->isChecked()) {
            procesadaImagen = escala_grises(procesadaImagen);
        }

        if(ui->recortar_bt->isChecked()) {
            procesadaImagen = recortar_video(procesadaImagen);
        }

        if(ui->zoom_bt->isChecked()) {
            procesadaImagen = recortar_alargar_video(procesadaImagen);
        }

        if(ui->grabar_bt->isChecked()) {
            grabar(procesadaImagen);
        }
    }
}

void MainWindow::compute() {
    /* En condiciones de baja luz la cámara puede dar un framerate menor (mayor tiempo de exposición para obtener una imagen
     * menos oscura, haciendo que "*cam >> camImage" tarde más, por lo que si aún no hay una imagen disponible
     * utilizamos la última que obtuvimos */

    std::vector<int> ready_index;
    if (VideoCapture::waitAny({*cam}, ready_index, 1000000)) { // Espera durante 1 milisegundo
        // La cámara está lista; Obtenemos la imagen.
        *cam >> camImagen;
        cv::cvtColor(camImagen, camImagen, cv::COLOR_BGR2RGB);
    }
    cv::resize(camImagen, origenImagen, Size(320,240));

    // Procesamos la imagen
    if(ui->pausar_bt->isChecked() == false) {
        procesar_Video();
    }

    // Escribimos en el flujo de salida
    Mat dest;
    cvtColor(procesadaImagen, dest, COLOR_RGB2YUV_I420);
    flip(dest, dest, 1);
    write(output, dest.data, framesize);

    // Adaptamos la imagen al visor y los actualizamos
    cv::resize(procesadaImagen, destinoImagen, Size(320,240));

    if(winSelected) {
        visorS->drawSquare(QRect(imageWindow.x, imageWindow.y, imageWindow.width,imageWindow.height), Qt::green );
    }

    visorS->update();
    visorD->update();
}

void MainWindow::selectWindow(QPointF p, int w, int h)
{
    QPointF pEnd;
    if(w>0 && h>0)
    {
        imageWindow.x = int(p.x()-w/2);
        if(imageWindow.x<0)
            imageWindow.x = 0;
        imageWindow.y = int(p.y()-h/2);
        if(imageWindow.y<0)
            imageWindow.y = 0;
        pEnd.setX(p.x()+w/2);
        if(pEnd.x()>=320)
            pEnd.setX(319);
        pEnd.setY(p.y()+h/2);
        if(pEnd.y()>=240)
            pEnd.setY(239);
        imageWindow.width = int(pEnd.x()-imageWindow.x+1);
        imageWindow.height = int(pEnd.y()-imageWindow.y+1);

        winSelected = true;
    }
}

void MainWindow::deselectWindow()
{
    winSelected = false;
}

MainWindow::~MainWindow()
{
    delete ui;
    delete cam;
    delete visorS;
    delete visorD;
    origenImagen.release();
    destinoImagen.release();
    camImagen.release();
    procesadaImagen.release();
    ::close(output);
}

