#include "operations.h"

Mat escala_grises( Mat img )
{
    Mat aux;
    cvtColor( img, aux, COLOR_RGB2GRAY );
    cvtColor( aux, aux, COLOR_GRAY2RGB );
    return aux;
}

Mat recortar_video( Mat img, Rect recorte )
{
    Mat aux = Mat( img, recorte );
    return aux;
}

Mat recortar_video_relativo( Mat img, QRectF recorte )
{
    Rect recorte_abs;
    // Se calculan las coordenadas absolutas
    recorte_abs.x = round( recorte.x() * img.cols );
    recorte_abs.y = round( recorte.y() * img.rows );
    recorte_abs.width = round( recorte.width() * img.cols );
    recorte_abs.height = round( recorte.height() * img.rows );

    return recortar_video( img, recorte_abs );
}

Mat ecualizado( Mat img )
{
    Mat aux, channelsYUV[3];
    // Convertimos la imagen a formato YUV y aplicamos el ecualizado al primer canal (el de luminosidad)
    cvtColor( img, aux, COLOR_RGB2YUV );
    split( aux, channelsYUV );
    equalizeHist( channelsYUV[0], channelsYUV[0] );
    merge( channelsYUV, 3, aux );
    cvtColor( aux, aux, COLOR_YUV2RGB );
    return aux;
}

Mat filtro_mediana( Mat img )
{
    Mat aux;
    medianBlur( img, aux, 5 );
    return aux;
}

Mat filtro_afilado( Mat img )
{
    Mat aux;
    float kernel_values[9] = {0, -1,  0,
                              -1,  5, -1,
                              0, -1,  0
                             };
    Mat kernel = Mat( 3, 3, CV_32F, kernel_values );
    filter2D( img, aux, -1, kernel );
    return aux;
}

Mat erisionar( Mat img )
{
    Mat aux;
    erode( img, aux, getStructuringElement( MORPH_RECT, Size( 3, 3 ) ) );
    return aux;
}

Mat pixelar( Mat img, QRectF area )
{
    // Se calculan las coordenadas absolutas
    Rect area_abs;
    area_abs.x = round( area.x() * img.cols );
    area_abs.y = round( area.y() * img.rows );
    area_abs.width = round( area.width() * img.cols );
    area_abs.height = round( area.height() * img.rows );

    // Pixelamos el area
    Mat aux = img.clone();
    Mat img_recortada = Mat( img, area_abs );
    cv::resize( img_recortada, img_recortada, Size( area_abs.width / 12, area_abs.height / 12 ), 0, 0, INTER_LINEAR );
    cv::resize( img_recortada, img_recortada, Size( area_abs.width, area_abs.height ), 0, 0, INTER_NEAREST );
    img_recortada.copyTo( aux( area_abs ) );
    return aux;
}

Mat rotation_transformation( Mat img, int val, QPointF centro )
{
    Point centro_abs = Point( img.cols * centro.x(), img.rows * centro.y() );
    float escala = 1;

    // Si la imagen se está rotando un ángulo de entre 60º y 120º reducimos la escala para que entre verticalmente
    if ( abs( val % 180 ) >= 60 && abs( val % 180 ) <= 120 )
        escala = float( img.rows ) / img.cols;

    // Calculamos la rotación
    Mat rot_mat = getRotationMatrix2D( centro_abs, val, escala );

    // Obtenemos la nueva imagen
    Mat aux;
    warpAffine( img, aux, rot_mat, img.size() );
    return aux;
}

Mat perspective_transformation( Mat img, QPointF p[4], QRectF section )
{
    // Ordenamos las coordenadas para que p_orden[3] sea la superior izquiera, [2] la inferior izquierda, [1] la superior derecha y [0] la inferior derecha
    QPointF p_orden [4];
    for ( int i = 0; i < 4; i++ )
    {
        // Calculamos cuantos puntos hay a la derecha y abajo de nuestro punto p[i]
        int derecha = 0, abajo = 0;
        for ( int j = 1; j < 4; j++ )
        {
            if ( p[i].x() < p[( i + j ) % 4].x() )
                derecha++;
            if ( p[i].y() < p[( i + j ) % 4].y() )
                abajo++;
        }

        // Hallamos la posición del array en la que tiene que estar el punto y calculamos las coordenadas absolutas
        int posicion = ( derecha / 2 ) * 2 + ( abajo / 2 );
        QPointF p_add = QPointF( p[i].x() * img.cols, p[i].y() * img.rows );
        if ( p_orden[posicion] == QPointF() )
        {
            p_orden[posicion] = p_add;
        }
        else
        {
            // En el caso de que ya haya un punto en esa posición comprobamos cual de los dos puntos está más arriba o abajo
            // en función de la posición (si es superior o inferior)
            if ( ( posicion % 2 == 0 && abajo == 0 ) || ( posicion % 2 == 1 && abajo == 3 ) )
            {
                p_orden[abajo == 0 ? posicion + 1 : posicion - 1 ] = p_orden[posicion];
                p_orden[posicion] = p_add;
            }
            else
            {
                p_orden[abajo == 0 ? posicion + 1 : posicion - 1 ] = p_add;
            }
        }
    }

    // Calculamos las coordenadas absolutas de la sección donde hay que centrar el resultado
    QRect section_abs;
    section_abs.setX( section.x() * img.cols );
    section_abs.setWidth( section.width() * img.cols );
    section_abs.setY( section.y() * img.rows );
    section_abs.setHeight( section.height() * img.rows );

    // Calculamos el rectángulo más pequeño que contenga los 4 puntos seleccionados
    int x = min( {p_orden[0].x(), p_orden[1].x(), p_orden[2].x(), p_orden[3].x()} );
    int y = min( {p_orden[0].y(), p_orden[1].y(), p_orden[2].y(), p_orden[3].y()} );
    int width = max( {p_orden[0].x(), p_orden[1].x(), p_orden[2].x(), p_orden[3].x()} ) - x;
    int height = max( {p_orden[0].y(), p_orden[1].y(), p_orden[2].y(), p_orden[3].y()} ) - y;

    /* Reescalamos nuestro rectangulo (que contiene los puntos) para que quepa en la sección "section" y calculamos
       el desplazamiento que tiene que tener cada punto tras la transformación
    */
    pair<int, int> dimensiones_adapt = adapt_size( pair<int, int> ( width, height ), section_abs.width(), section_abs.height(), true );
    int diferencia_x = ( dimensiones_adapt.first - width ) / 2;
    int diferencia_y = ( dimensiones_adapt.second - height ) / 2;

    // Calculamos la distancia que hay que desplazar los puntos para que la selección (formada por los cuatro puntos) quede centrada en la sección "section"
    int desp_x = ( section_abs.x() + ( section_abs.width() / 2 ) - ( width / 2 ) ) - x;
    int desp_y = ( section_abs.y() + ( section_abs.height() / 2 ) - ( height / 2 ) ) - y;
    for ( int i = 0; i < 4; i++ )
    {
        p_orden[i].setX( p_orden[i].x() + desp_x );
        p_orden[i].setY( p_orden[i].y() + desp_y );
    }

    // Calculamos los puntos que conforman el rectángulo el cual contiene nuestra area seleccionada junto con un margen alrededor
    int area_recortar_ini_x = max( 0, -desp_x );
    int area_recortar_ini_y = max( 0, -desp_y );
    int area_recortar_fin_x = min( img.cols - 1, img.cols - desp_x );
    int area_recortar_fin_y = min( img.rows - 1, img.rows - desp_y );
    Rect area_recortar = Rect( Point( area_recortar_ini_x, area_recortar_ini_y ), Point( area_recortar_fin_x, area_recortar_fin_y ) );

    // Recortamos el área anteriormente citada y la colocamos centrada (respecto a "section") sobre una imagen negra
    Mat aux = Mat( img.size(), img.type(), Scalar( 0, 0, 0 ) );
    Mat img_recortada = Mat( img, area_recortar );
    area_recortar.x += desp_x;
    area_recortar.y += desp_y;
    img_recortada.copyTo( aux( area_recortar ) );

    // Los puntos sin el número (a,b...) representan las 4 esquinas de la imagen original
    // Los otros puntos (a2,b2...) representan la posición de las 4 esquinas tras la transformación
    Point2f a  = Point2f( p_orden[3].x(), p_orden[3].y() );
    Point2f a2 = Point2f( p_orden[3].x() - diferencia_x, p_orden[3].y() - diferencia_y );

    Point2f b  = Point2f( p_orden[1].x(), p_orden[1].y() );
    Point2f b2 = Point2f( p_orden[1].x() + diferencia_x, p_orden[1].y() - diferencia_y );

    Point2f c  = Point2f( p_orden[2].x(), p_orden[2].y() );
    Point2f c2 = Point2f( p_orden[2].x() - diferencia_x, p_orden[2].y() + diferencia_y );

    Point2f d  = Point2f( p_orden[0].x(), p_orden[0].y() );
    Point2f d2 = Point2f( p_orden[0].x() + diferencia_x, p_orden[0].y() + diferencia_y );

    // Calculamos la transformación de perspectiva
    Point2f from [4] = { a,  b,  c,  d  };
    Point2f to   [4] = { a2, b2, c2, d2 };
    Mat perspect_mat = getPerspectiveTransform( from, to );

    // Obtenemos la nueva imagen
    warpPerspective( aux, aux, perspect_mat, img.size() );
    return aux;
}

Mat control_RGB( Mat img, int red, int green, int blue )
{
    Mat aux, canales[3];
    split( img, canales );
    canales[0] = canales[0] * ( red / 100.0f );
    canales[1] = canales[1] * ( green / 100.0f );
    canales[2] = canales[2] * ( blue / 100.0f );
    merge( canales, 3, aux );
    return aux;
}

Window *get_win_list( Display *disp, unsigned long *len )
{
    int format;
    unsigned long bytes;
    unsigned char *data;
    Atom type;

    Atom property = XInternAtom( disp, "_NET_CLIENT_LIST", false );

    if ( XGetWindowProperty( disp, XDefaultRootWindow( disp ), property, 0, 1024, false, XA_WINDOW,
                             &type, &format, len, &bytes, &data ) != Success )
    {
        return NULL;
    }

    return ( Window* )data;
}

char *get_win_name( Display *disp, Window win )
{
    int format;
    unsigned long bytes, len;
    unsigned char *data;
    Atom type;

    Atom property = XInternAtom( disp, "WM_NAME", false );

    if ( XGetWindowProperty( disp, win, property, 0, 1024, false, XA_STRING,
                             &type, &format, &len, &bytes, &data ) != Success )
    {
        return NULL;
    }

    return ( char* )data;
}

pair<int, int> adapt_size( pair<int, int> size, int limit_width, int limit_height, bool expand )
{
    int actual_width = size.first;
    int actual_height = size.second;

    if ( actual_width > limit_width or actual_height > limit_height or expand )
    {
        int new_Width, new_Height;

        // La imagen se sale de la pantalla por lo que hay que ajustar
        if ( ( limit_height * actual_width / actual_height ) <= limit_width )
        {
            // Se sale mas de alto que de ancho proporcionalmente
            new_Height = limit_height;
            new_Width = new_Height * actual_width / actual_height;
        }
        else
        {
            // Se sale mas de ancho que de alto proporcionalmente
            new_Width = limit_width;
            new_Height = new_Width * actual_height / actual_width;
        }

        return make_pair( new_Width, new_Height );
    }
    return make_pair( actual_width, actual_height );
}

pair<int, int> get_win_size( Display *disp, long long win )
{
    Mat a;
    if ( get_screen_frame( disp, win, &a ) )
    {
        return make_pair( a.cols, a.rows );
    }
    else
    {
        return make_pair( -1, -1 );
    }
}

pair<int, int> get_cam_size( VideoCapture *cam )
{
    if ( cam->isOpened() )
    {
        return make_pair( cam->get( CAP_PROP_FRAME_WIDTH ), cam->get( CAP_PROP_FRAME_HEIGHT ) );
    }
    else
    {
        return make_pair( -1, -1 );
    }
}

int get_screens()
{
    return QGuiApplication::screens().size();
}

bool get_cam_frame( VideoCapture *cam, Mat *image )
{
    if ( not cam->isOpened() )
        return false;

    /* En condiciones de baja luz la cámara puede dar un framerate menor (mayor tiempo de exposición para obtener una imagen
     * menos oscura, haciendo que "*cam >> *image" tarde más, por lo que si aún no hay una imagen disponible
     * no cambiamos *image
     */

    vector<int> ready_index;
    bool cam_ready;

    try
    {
        // Espera durante 1 milisegundo
        cam_ready = VideoCapture::waitAny( {*cam}, ready_index, 1000000 );
    }
    catch ( Exception e )
    {
        return false;
    }

    if ( cam_ready )
    {
        // La cámara está lista; Obtenemos la imagen.
        *cam >> *image;
        cvtColor( *image, *image, COLOR_BGR2RGB );
    }

    return true;
}

bool get_screen_frame( Display *disp, long long win, Mat *image )
{
    QScreen* screen;

    /* Si wind es negativo o 0 es una captura de la pantalla completa.
     * wind contendrá el id de la pantalla negado, esto es, para la pantalla 1 valdrá -1
     */
    if ( win <= 0 )
    {
        // Se comprueba si el número es válido
        if ( get_screens() > - win )
        {
            // Seleccionamos la pantalla correspondiente y ponemos wind a 0 para que capture la pantalla completa
            screen = QGuiApplication::screens().at( - win );
            win = 0;
        }
        else
        {
            return false;
        }
    }
    else   // Hay que capturar una ventana
    {
        // Se comprueba si la ventana wind sigue existiendo
        unsigned long len;
        bool found = false;

        Window *list = get_win_list( disp, &len );

        if ( list == NULL )
            return false;

        for ( int i = 0; i < ( int )len; i++ )
        {
            if ( ( Window ) win == list[i] )
            {
                found = true;
                break;
            }
        }

        if ( found == false )
            return false;

        // Ponemos la pantalla por defecto
        screen = QGuiApplication::primaryScreen();
    }

    // Obtenemos un pixmap de la ventana wind y lo pasamos a un QImage
    QPixmap pixmap = screen->grabWindow( ( Window ) win );
    QImage qimg = pixmap.toImage();

    // Comprobamos si hemos podido obtener una imágen de la ventana
    if ( qimg.isNull() )
    {
        return false;
    }
    else
    {
        // Realizamos la conversión de QImage a Mat
        *image = Mat( qimg.height(), qimg.width(), CV_8UC4, const_cast<uchar*>( qimg.bits() ), qimg.bytesPerLine() );
        cvtColor( *image, *image, COLOR_BGRA2RGB );
    }
    return true;
}

int set_output_format( int output, int width, int height )
{
    // Comprobamos que el dispositivo de salida está abierto
    if ( output < 0 )
    {
        cerr << "ERROR: No se ha podido abrir el dispositivo de salida: " << strerror( errno ) << "\n";
        return -1;
    }

    struct v4l2_format format_vid;
    memset( &format_vid, 0, sizeof( format_vid ) );
    format_vid.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;

    // Obtenemos el formato del dispositivo
    if ( ioctl( output, VIDIOC_G_FMT, &format_vid ) < 0 )
    {
        cerr << "ERROR: No se ha podido obtener el formato de video: " << strerror( errno ) << "\n";
        return -2;
    }

    // Configuramos el formato del video de salida
    format_vid.fmt.pix.width = width;
    format_vid.fmt.pix.height = height;
    size_t framesize = width * height * 1.5;
    format_vid.fmt.pix.sizeimage = framesize;
    format_vid.fmt.pix.pixelformat = V4L2_PIX_FMT_YUV420;
    format_vid.fmt.pix.field = V4L2_FIELD_NONE;

    // Establecemos el formato
    if ( ioctl( output, VIDIOC_S_FMT, &format_vid ) < 0 )
    {
        cerr << "ERROR: No se ha podido establecer el formato: " << strerror( errno ) << "\n";
        return -3;
    }

    // Obtenemos el formado actual del dispositivo
    if ( ioctl( output, VIDIOC_G_FMT, &format_vid ) < 0 )
    {
        cerr << "ERROR: No se ha podido obtener el formato de video: " << strerror( errno ) << "\n";
        return -2;
    }

    // Comprobamos que el nuevo formato corresponda con el width y height pasados
    if ( ( int ) format_vid.fmt.pix.width != width || ( int ) format_vid.fmt.pix.height != height )
    {
        return -4;
    }

    return  0;
}
