#include "fcnmask.h"

fcnmask::fcnmask( int width, int height )
{
    // Configuración de la red neuronal
    try
    {
        network = dnn::readNetFromCaffe( "../TFG/netData/fcn.prototxt", "../TFG/netData/fcn.caffemodel" );
        network.setPreferableTarget( dnn::DNN_TARGET_CUDA );
        network.setPreferableBackend( dnn::DNN_BACKEND_CUDA );

        status = 0;
    }
    catch ( Exception )
    {
        status = -1;
    }

    result_mask = Mat( Size( width, height ), CV_8UC1, Scalar( 0 ) );
    result_img = Mat( Size( width, height ), CV_8UC3, Scalar( 0, 0, 0 ) );
    original_img = Mat( Size( width, height ), CV_8UC3, Scalar( 0, 0, 0 ) );

    this->height = height;
    this->width = width;
}

fcnmask::~fcnmask()
{
    result_mask.release();
    result_img.release();
    original_img.release();
}

void fcnmask::stop()
{
    status = 0;
}

int fcnmask::get_status()
{
    return status;
}

void fcnmask::apply_network_FCN()
{
    int width_reduced = 160;
    int height_reduced = height * width_reduced / width;

    Mat resultados, img, input, output;
    Scalar media;

    result_mask = Mat( Size( width, height ), CV_8UC1, Scalar( 0 ) );
    result_img = Mat( Size( width, height ), CV_8UC3, Scalar( 0, 0, 0 ) );
    original_img = Mat( Size( width, height ), CV_8UC3, Scalar( 0, 0, 0 ) );

    resultados.create( height_reduced, width_reduced, CV_8UC1 );

    status = 1;
    while ( status == 1 )
    {
        img = original_img.clone();

        cvtColor( img, img, COLOR_RGB2BGR );

        // Usamos la imagen como input de la red neuronal
        media = mean( img );
        input = dnn::blobFromImage( img, 1, Size( width_reduced, height_reduced ), media );

        cvtColor( img, img, COLOR_BGR2RGB );

        network.setInput( input );
        output = network.forward();

        /* Miramos en cada pixel cual es la categoría más probable y creamos una máscara con los pixeles donde
         * el humano es la categoría predominante
         */
        for ( int i = 0; i < height_reduced; i++ )
        {
            for ( int j = 0; j < width_reduced; j++ )
            {
                int categoria = -1;
                float maxima_prob = -1;
                // Se comprueba la probabilidad que da la red para cada una de las 21 categorias en las que segmenta
                for ( int k = 0; k < 21; k++ )
                {
                    int id_Array[4] = {0, k, i, j};
                    float val = output.at<float>( id_Array );
                    // Se guarda la categoria con mayor probabilidad
                    if ( val > maxima_prob )
                    {
                        maxima_prob = val;
                        categoria = k;
                    }
                }
                resultados.at<uchar>( i, j ) = ( categoria == 15 ? 255 : 0 ); // Si la categoria del pixel es humano (15) formará parte de la máscara
            }
        }

        // Se reescala el resultado
        resize( resultados, result_mask, Size( width, height ), INTER_NEAREST );
        result_img = img.clone();
    }
}

Mat fcnmask::get_mask( Mat img )
{
    // Calculamos la altura y anchura con la que trabajaremos para reducir la carga computacional
    int width_reduced = 160;
    int height_reduced = height * width_reduced / width;

    /* Copiamos los recursos que utilizaremos para evitar perderlos si la red neuronal los sobreescribe
     * También copiaremos la imagen actual a original_img para que la red neuronal trabaje sobre ella
     */
    Mat img_ant = result_img.clone();
    Mat mask = result_mask.clone();
    original_img = img.clone();

    // Modificamos los Mats para poder trabajar con ellos, convirtiendolos en gris y reescalandolos
    cvtColor( img_ant, img_ant, COLOR_RGB2GRAY );
    resize( img_ant, img_ant, Size( width_reduced, height_reduced ), INTER_NEAREST );

    cvtColor( img, img, COLOR_RGB2GRAY );
    resize( img, img, Size( width_reduced, height_reduced ), INTER_NEAREST );

    resize( mask, mask, Size( width_reduced, height_reduced ), INTER_NEAREST );

    /* Calculamos el flow, el movimiento que ha habido de img a img_ant, esto es así porque no necesitamos
     * saber el movimiento de los pixeles (como se han movido de img_ant a img) sino su origen
     * (¿este pixel de img_ant donde estaba en img?)
     */
    Mat flow, new_Mask;
    calcOpticalFlowFarneback( img, img_ant, flow, 0.70, 12, 40, 12, 10, 2, 0 );

    /* Para poder usar la función remap es necesario calcular un map usando el flow y compensarlo cada vector de flujo
     * con su ubicación (x, y)
     */
    Mat map( flow.size(), CV_32FC2 );
    for ( int x = 0; x < flow.cols; x++ )
    {
        for ( int y = 0; y < flow.rows; y++ )
        {
            Point2f f = flow.at<Point2f>( y, x );
            map.at<Point2f>( y, x ) = Point2f( x + f.x, y + f.y );
        }
    }

    // Se aplica el movimiento que ha habido (almacenado en map) a la mascara (mask) y se almacena en
    remap( mask, new_Mask, map, Mat(), INTER_LINEAR );

    // Se reescala el resultado y es devuelto
    resize( new_Mask, new_Mask, Size( width, height ), INTER_NEAREST );

    return new_Mask;
}
