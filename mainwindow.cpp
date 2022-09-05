#include "mainwindow.h"
#include "ui_mainwindow.h"

#define VIDEO_OUT  12

MainWindow::MainWindow( QWidget *parent )
    : QMainWindow( parent )
    , ui( new Ui::MainWindow )
{
    ui->setupUi( this );

    QErrorMessage error_msg( this );

    win_selected = false;
    ui->Panel_Editor->setHidden( true );
    ui->Escena_Renombrar->setHidden( true );

    // Configuramos la combobox de la resolución y seleccionamos una por defecto
    for ( int i = 0; i < ( int ) resoluciones.size(); i++ )
    {
        int width = resoluciones.at( i ).first;
        int height = resoluciones.at( i ).second;
        ui->Resolucion_ComboBox->addItem( QString::number( width ) + " x " + QString::number( height ) );
    }
    ui->Resolucion_ComboBox->setCurrentIndex( 1 );

    // Abrimos la salida y la configuramos
    int status = establecer_resolucion( ui->Resolucion_ComboBox->currentIndex() );

    if ( status < 0 )
        return;

    win_resize = true;
    visor = new imageviewer( ui->imageFrame, &visor_image );

    /* Añadimos un menu vacio al botón de añadir elemento para que aparezca como un menú desplegable, cuando el
     * usuario haga clic en el botón el método "preparar_anadir_elto_Btn_Signal" se encargará de configurarlo
     */
    menu_add_elto = new QMenu();
    ui->Anadir_Elto_Btn->setMenu( menu_add_elto );

    display = XOpenDisplay( NULL );
    if ( display == NULL )
    {
        cerr << "ERROR: al abrir el display" << "\n";
        return;
    }

    timer.setTimerType( Qt::PreciseTimer );
    timer.start( 1.0 / ui->Framerate_SpinBox->value() * 1000 );

    // Conexiones con los slots
    connect( &timer, SIGNAL( timeout() ), this, SLOT( compute_Signal() ) );

    /* --- Visor --- */
    connect( visor, SIGNAL( windowSelected( QRect ) ), this, SLOT( select_window_Signal( QRect ) ) );
    connect( visor, SIGNAL( windowCancel() ), this, SLOT( deselect_window_Signal() ) );
    connect( visor, SIGNAL( win_interact_start( QPoint ) ), this, SLOT( window_interact_start_Signal( QPoint ) ) );
    connect( visor, SIGNAL( win_interact_update( QPoint ) ), this, SLOT( window_interact_update_Signal( QPoint ) ) );

    /* --- Ventana editar --- */
    connect( ui->Modo_Color_Btn, SIGNAL( clicked( bool ) ), this, SLOT( modo_color_Btn_Signal() ) );
    connect( ui->Detectar_Fondo_Btn, SIGNAL( clicked( bool ) ), this, SLOT( detect_fondo_Btn_Signal( bool ) ) );
    connect( ui->Modo_Fondo_Btn, SIGNAL( clicked( bool ) ), this, SLOT( modo_fondo_Btn_Signal() ) );
    connect( ui->Recortar_Btn, SIGNAL( clicked( bool ) ), this, SLOT( recortar_Btn_Signal() ) );
    connect( ui->Eliminar_Recorte_Btn, SIGNAL( clicked( bool ) ), this, SLOT( eliminar_recorte_Btn_Signal() ) );
    connect( ui->Perspectiva_Btn, SIGNAL( clicked( bool ) ), this, SLOT( perspectiva_Btn_Signal( ) ) );
    connect( ui->Aspect_ratio_CheckBox, SIGNAL( clicked( bool ) ), this, SLOT( aspect_ratio_Checkbox_Signal( bool ) ) );
    connect( ui->Pixelacion_Btn, SIGNAL( clicked( bool ) ), this, SLOT( pixelacion_Btn_Signal( ) ) );
    connect( ui->Imagen_Fondo_Add_Btn, SIGNAL( clicked( bool ) ), this, SLOT( imagen_fondo_Btn_Signal( ) ) );
    connect( ui->Salir_Btn, SIGNAL( clicked( bool ) ), this, SLOT( salir_Btn_Signal() ) );

    /* --- Elementos --- */
    connect( ui->Elementos_Lista, SIGNAL( currentRowChanged( int ) ), this, SLOT( elementos_lista_Select_Signal( int ) ) );
    connect( menu_add_elto, SIGNAL( triggered( QAction* ) ), this, SLOT( anadir_elto_Btn_Signal( QAction* ) ) );
    connect( menu_add_elto, SIGNAL( aboutToShow() ), this, SLOT( preparar_anadir_elto_Btn_Signal() ) );
    connect( ui->Configurar_Elto_Btn, SIGNAL( clicked( bool ) ), this, SLOT( configurar_elto_Btn_Signal() ) );
    connect( ui->Borrar_Elto_Btn, SIGNAL( clicked( bool ) ), this, SLOT( borrar_elto_Btn_Signal() ) );

    /* --- Escenas --- */
    connect( ui->Escenas_Lista, SIGNAL( currentRowChanged( int ) ), this, SLOT( escenas_lista_Select_Signal( int ) ) );
    connect( ui->Anadir_Escena_Btn, SIGNAL( clicked( bool ) ), this, SLOT( anadir_escena_Btn_Signal() ) );
    connect( ui->Borrar_Escena_Btn, SIGNAL( clicked( bool ) ), this, SLOT( borrar_escena_Btn_Signal() ) );
    connect( ui->Renombrar_Escena_Btn, SIGNAL( clicked( bool ) ), this, SLOT( renombrar_escena_Btn_Signal() ) );
    connect( ui->Cancelar_Renombrar_Btn, SIGNAL( clicked( bool ) ), this, SLOT( cancelar_renombrar_escena_Btn_Signal() ) );
    connect( ui->Aceptar_Renombrar_Btn, SIGNAL( clicked( bool ) ), this, SLOT( aceptar_renombrar_escena_Btn_Signal() ) );
    connect( ui->Renombrar_Escena_TextEdit, SIGNAL( textChanged() ), this, SLOT( renombrar_Changed_Signal() ) );
    connect( ui->Exportar_Escena_Btn, SIGNAL( clicked( bool ) ), this, SLOT( exportar_escena_Btn_Signal() ) );
    connect( ui->Importar_Escena_Btn, SIGNAL( clicked( bool ) ), this, SLOT( importar_escena_Btn_Signal() ) );

    /* --- Acciones elementos --- */
    connect( ui->Flecha_Arriba, SIGNAL( clicked( bool ) ), this, SLOT( flecha_arriba_Btn_Signal() ) );
    connect( ui->Ocultar_Elto_Btn, SIGNAL( clicked( bool ) ), this, SLOT( ocultar_elto_Btn_Signal() ) );
    connect( ui->Flecha_Abajo, SIGNAL( clicked( bool ) ), this, SLOT( flecha_abajo_Btn_Signal() ) );

    /* --- Acciones generales --- */
    connect( ui->Pausa_Btn, SIGNAL( clicked( bool ) ), this, SLOT( pausa_Btn_Signal( bool ) ) );
    connect( ui->Framerate_SpinBox, SIGNAL( valueChanged( int ) ), this, SLOT( framerate_Signal( int ) ) );
    connect( ui->Resolucion_ComboBox, SIGNAL( activated( int ) ), this, SLOT( resolucion_Signal( int ) ) );
}



/// -*-*-*- PROCESAMIENTO -*-*-*-

void MainWindow::mem_to_ui()
{
    elemento_visual *elto_vi = &lista_eltos_visuales.at( escena_index ).at( elemento_index );

    // Pasamos los valores a la ui
    ui->X_SpinBox->setValue( elto_vi->x );
    ui->Y_SpinBox->setValue( elto_vi->y );
    ui->Altura_SpinBox->setValue( elto_vi->height );
    ui->Anchura_SpinBox->setValue( elto_vi->width );
    ui->Rojo_Slider->setValue( elto_vi->R );
    ui->Verde_Slider->setValue( elto_vi->G );
    ui->Azul_Slider->setValue( elto_vi->B );
    ui->Filtro_Mediana_Btn->setChecked( elto_vi->filtro_mediana );
    ui->Ecualizado_Btn->setChecked( elto_vi->ecualizado );
    ui->Filtro_Afilado_Btn->setChecked( elto_vi->filtro_afilado );
    ui->Erosionar_Btn->setChecked( elto_vi->erosionar );
    ui->Detectar_Fondo_Btn->setChecked( elto_vi->detectar_fondo );
    ui->Rotacion_Slider->setValue( elto_vi->rotacion_actual );
}

void MainWindow::ui_to_mem()
{
    elemento_visual *elto_vi = &lista_eltos_visuales.at( escena_index ).at( elemento_index );

    // Guardamos los valores de la ui
    elto_vi->x = ui->X_SpinBox->value();
    elto_vi->y = ui->Y_SpinBox->value();
    elto_vi->height = ui->Altura_SpinBox->value();
    elto_vi->width = ui->Anchura_SpinBox->value();
    elto_vi->R = ui->Rojo_Slider->value();
    elto_vi->G = ui->Verde_Slider->value();
    elto_vi->B = ui->Azul_Slider->value();
    elto_vi->filtro_mediana = ui->Filtro_Mediana_Btn->isChecked();
    elto_vi->ecualizado = ui->Ecualizado_Btn->isChecked();
    elto_vi->filtro_afilado = ui->Filtro_Afilado_Btn->isChecked();
    elto_vi->erosionar = ui->Erosionar_Btn->isChecked();
    elto_vi->detectar_fondo = ui->Detectar_Fondo_Btn->isChecked();
    elto_vi->rotacion_actual = ui->Rotacion_Slider->value();
}

vector<string> MainWindow::split_string( string texto, char delimitador )
{
    vector<string> elementos;
    stringstream texto_stream( texto );
    string str;
    while ( getline( texto_stream, str, delimitador ) )
    {
        elementos.push_back( str );
    }
    return elementos;
}

QPointF MainWindow::coords_relativas_elemento( elemento_visual elto, QPointF point, bool modo )
{
    QPoint inicio_elto = QPoint( elto.x, elto.y );
    float width_elto = elto.width;
    float height_elto = elto.height;

    if ( elto.recorte.isEmpty() == false )
    {
        width_elto = width_elto / elto.recorte.width();
        height_elto = height_elto / elto.recorte.height();
        inicio_elto.setX( inicio_elto.x() - elto.recorte.x() * width_elto );
        inicio_elto.setY( inicio_elto.y() - elto.recorte.y() * height_elto );
    }

    if ( modo )
        return QPointF( ( point.x() - inicio_elto.x() ) / width_elto, ( point.y() - inicio_elto.y() ) / height_elto );

    return QPointF( point.x() * width_elto + inicio_elto.x(), point.y() * height_elto + inicio_elto.y() );
}

QRect MainWindow::win_seleccion_absoluta( elemento_visual elto )
{
    if ( win_selected == false )
        return QRect();

    // Convertimos las coordenadas de la selección (del visor) en coordenadas de la imagen de salida
    QRect rect_Adapt = convertir_coords( selected_rect, true );

    /* Hallamos la intersección entre la selección del usuario y la imagen
     * Si la intersección es inválida (el ancho o alto es 0) entonces devolvemos un QRect vacío
     */
    QRect intersect = rect_Adapt.intersected( QRect( elto.x, elto.y, elto.width, elto.height ) );
    if ( intersect.width() <= 0 || intersect.height() <= 0 )
        return QRect();

    /* Calculamos las coordenadas de la selección (respecto de la imagen):
     * x e y representan a cuantos pixeles del comienzo de la imagen inicia la selección
     * el width y height representan el ancho y alto de la selección
     */
    QRect rect;
    rect.setX( intersect.x() - elto.x );
    rect.setY( intersect.y() - elto.y );
    rect.setWidth( min( intersect.width(), elto.width - rect.x() ) );
    rect.setHeight( min( intersect.height(), elto.height - rect.y() ) );

    return rect;
}

QRectF MainWindow::win_seleccion_relativa( elemento_visual elto )
{

    QRect rect_abs = win_seleccion_absoluta( elto );

    if ( rect_abs.isEmpty() )
        return QRectF();

    /* Hallamos el ancho y alto total de la imagen (incluido lo que está recortado) para
     * tener los porcentajes de la misma base
     */
    int total_width = elto.width / ( elto.recorte.isEmpty() ? 1 : elto.recorte.width() );
    int total_height = elto.height / ( elto.recorte.isEmpty() ? 1 : elto.recorte.height() );

    // Calculamos las coordenadas de la selección en valor relativo
    QRectF rect;
    rect.setX( ( float( rect_abs.x() ) + floor( elto.recorte.isEmpty() ? 0 : elto.recorte.x() * total_width ) ) / total_width );
    rect.setY( ( float( rect_abs.y() ) + floor( elto.recorte.isEmpty() ? 0 : elto.recorte.y() * total_height ) ) / total_height );
    rect.setWidth( float( rect_abs.width() ) / total_width );
    rect.setHeight( float( rect_abs.height() ) / total_height );

    return rect;
}

void MainWindow::add_rotacion( elemento_visual *elto, int id_proces )
{
    // Configura el proceso "id_proces" para que se realice después de la rotación actual
    elto->orden_proces[id_proces] = ( int )elto->rotaciones.size();

    // Configuramos la rotación y la añadimos al vector de rotaciones
    rotacion r;
    r.cantidad = elto->rotacion_actual;
    r.centro = coords_relativas_elemento( *elto, QPointF( elto->x + ( elto->width / 2.0 ), elto->y + ( elto->height / 2.0 ) ), true );
    elto->rotaciones.push_back( r );

    // Reseteamos la rotación actual
    elto->rotacion_actual = 0;
    mem_to_ui();
}

QPointF MainWindow::transform_to_area_recorte( elemento_visual elto, QPointF point )
{
    if ( elto.recorte.isEmpty() )
        return point;

    /* Hallamos el ancho y alto total de la imagen (incluido lo que está recortado) para
     * tener los porcentajes de la misma base
     */
    int total_width = elto.width / ( elto.recorte.isEmpty() ? 1 : elto.recorte.width() );
    int total_height = elto.height / ( elto.recorte.isEmpty() ? 1 : elto.recorte.height() );

    // Ponemos el punto en coordenadas absolutas
    QPointF p_abs;
    p_abs.setX( point.x() * total_width );
    p_abs.setY( point.y() * total_height );

    // Obtenemos la región recortada (en coordenadas absolutas)
    QRect r = QRect();
    r.setX( elto.recorte.x() * total_width );
    r.setY( elto.recorte.y() * total_height );
    r.setWidth( elto.recorte.width() * total_width );
    r.setHeight( elto.recorte.height() * total_height );

    // Calcula las coordenadas relativas del punto con respecto al area recortada
    float x = fmax( 0, fmin( 1, ( p_abs.x() - r.x() ) / r.width() ) );
    float y = fmax( 0, fmin( 1, ( p_abs.y() - r.y() ) / r.height() ) );

    return QPointF( x, y );
}

QRectF MainWindow::transform_to_area_recorte( elemento_visual elto, QRectF rect )
{
    QPointF p1 = transform_to_area_recorte( elto, rect.topLeft() );
    QPointF p2 = transform_to_area_recorte( elto, rect.bottomRight() );
    return QRectF ( p1, p2 );
}

void MainWindow::transform_to_area_recorte(  elemento_visual elto, QPointF *point, QPointF *result, int n_puntos  )
{
    for ( int i = 0; i < n_puntos; i++ )
    {
        result[i] = transform_to_area_recorte( elto, point[i] );
    }
}

/* ---  Gestión interfaz  --- */

void MainWindow::gestion_interfaz_seccion_editar()
{
    elemento_visual *elto_actual = &lista_eltos_visuales.at( escena_index ).at( elemento_index );

    if ( elto_actual->escala_grises == true )
    {
        ui->Modo_Color_Btn->setText( "Modo a color" );
        ui->Azul_Slider->setEnabled( false );
        ui->Rojo_Slider->setEnabled( false );
        ui->Verde_Slider->setEnabled( false );
    }
    else
    {
        ui->Modo_Color_Btn->setText( "Escala de grises" );
        ui->Azul_Slider->setEnabled( true );
        ui->Rojo_Slider->setEnabled( true );
        ui->Verde_Slider->setEnabled( true );
    }

    if ( elto_actual->tipo_fondo == 0 )
    {
        ui->Modo_Fondo_Btn->setText( "Recortar Fondo" );
        ui->Imagen_Fondo_Add_Btn->setVisible( false );
    }
    else if ( elto_actual->tipo_fondo == 1 )
    {
        ui->Modo_Fondo_Btn->setText( "Desenfocar Fondo" );
        ui->Imagen_Fondo_Add_Btn->setVisible( false );
    }
    else
    {
        ui->Modo_Fondo_Btn->setText( "Imagen Fondo" );
        ui->Imagen_Fondo_Add_Btn->setVisible( true );
    }

    if ( elto_actual->recorte.isEmpty() == true )
    {
        ui->Eliminar_Recorte_Btn->setHidden( true );
    }
    else
    {
        ui->Eliminar_Recorte_Btn->setHidden( false );
    }

    if ( elto_actual->perspectiva[3] != QPointF() )
    {
        ui->Perspectiva_Btn->setText( "Restablecer Perspect." );
    }
    else if ( ui->Perspectiva_Btn->isChecked() == true )
    {
        ui->Perspectiva_Btn->setText( "Cancelar" );
    }
    else
    {
        ui->Perspectiva_Btn->setText( "Cambiar Perspectiva" );
    }

    if ( elto_actual->pixelacion.isEmpty() == true )
    {
        ui->Pixelacion_Btn->setText( "Pixelar Región" );
    }
    else
    {
        ui->Pixelacion_Btn->setText( "Cancelar Pixelación" );
    }

}

void MainWindow::gestion_interfaz_escena( bool update_lista_eltos, int seleccionar = -1 )
{
    if ( escena_index == -1 ) // No hay ninguna escena seleccionada
    {
        elemento_index = -1;
        ui->Elementos_Lista->clear();
        ui->Anadir_Elto_Btn->setEnabled( false );
        ui->Borrar_Escena_Btn->setEnabled( false );

        ui->Escena_Renombrar->setHidden( true );
        ui->Escena_Botones->setHidden( false );
        ui->Renombrar_Escena_Btn->setEnabled( false );
        ui->Exportar_Escena_Btn->setEnabled( false );
    }
    else   // Hay una escena seleccionada
    {
        ui->Anadir_Elto_Btn->setEnabled( true );
        ui->Borrar_Escena_Btn->setEnabled( true );

        if ( update_lista_eltos ) // Actualiza la lista de elementos, seleccionamos el elemento "seleccionar"
        {
            elemento_index = seleccionar;
            ui->Elementos_Lista->clear();

            // Listamos los elementos
            vector<elemento_visual> *lista_eltos = &lista_eltos_visuales.at( escena_index );
            for ( int i = 0; i < ( int )lista_eltos->size(); i++ )
            {
                ui->Elementos_Lista->addItem( lista_eltos->at( i ).nombre );

                // Si está oculto lo tachamos
                if ( lista_eltos->at( i ).ocultar )
                {
                    QFont fnt;
                    fnt.setStrikeOut( true );
                    ui->Elementos_Lista->item( i )->setFont( fnt );
                }
            }

            ui->Elementos_Lista->setCurrentRow( seleccionar );
        }

        ui->Renombrar_Escena_Btn->setEnabled( true );
        ui->Exportar_Escena_Btn->setEnabled( true );
    }
}

void MainWindow::gestion_interfaz_elto()
{
    if ( elemento_index == -1 ) // No hay ningún elemento seleccionado
    {
        ui->Configurar_Elto_Btn->setEnabled( false );
        ui->Borrar_Elto_Btn->setEnabled( false );
        ui->Flecha_Arriba->setEnabled( false );
        ui->Flecha_Abajo->setEnabled( false );
        ui->Ocultar_Elto_Btn->setEnabled( false );
    }
    else   // Hay un elemento seleccionado
    {
        ui->Ocultar_Elto_Btn->setEnabled( true );
        ui->Configurar_Elto_Btn->setEnabled( true );
        ui->Borrar_Elto_Btn->setEnabled( true );

        if ( elemento_index == 0 )
        {
            ui->Flecha_Arriba->setEnabled( false );
        }
        else
        {
            ui->Flecha_Arriba->setEnabled( true );
        }

        if ( elemento_index + 1 == ( int )lista_eltos_visuales.at( escena_index ).size() )
        {
            ui->Flecha_Abajo->setEnabled( false );
        }
        else
        {
            ui->Flecha_Abajo->setEnabled( true );
        }
    }
}

void MainWindow::gestion_interfaz( bool update_lista_eltos, int seleccionar = -1 )
{
    if ( editando_elto == true ) // Se está mostrando la sección de editar
    {
        gestion_interfaz_seccion_editar();
    }
    else   // Se está mostrando la sección general
    {
        gestion_interfaz_escena( update_lista_eltos, seleccionar );
        gestion_interfaz_elto();
    }
}

/* ---        Visor       --- */

void MainWindow::resizeEvent( QResizeEvent* event )
{
    win_selected = false;
    win_resize = true;
}

void MainWindow::reescalar_visor()
{
    int hg, wd;
    int x = 0, y = 0;

    // Calculamos el punto inicial, el ancho y el alto del visor
    if ( ui->imageFrame->height() * VID_WIDTH < ui->imageFrame->width() * VID_HEIGHT )
    {
        hg = ui->imageFrame->height();
        wd = ui->imageFrame->height() * VID_WIDTH / VID_HEIGHT;
        x = ( ui->imageFrame->width() - wd ) / 2;
    }
    else
    {
        wd = ui->imageFrame->width();
        hg = ui->imageFrame->width() * VID_HEIGHT / VID_WIDTH;
        y = ( ui->imageFrame->height() - hg ) / 2;
    }

    // Creamos el mat con las nuevas dimensiones y lo asignamos al visor
    visor_image.release();
    visor_image.create( hg, wd, CV_8UC1 );
    visor_image = Scalar( 0, 0, 0 );

    visor->set_image( &visor_image );
    visor->set_position( x, y );
}

QPoint MainWindow::convertir_coords( QPoint point, bool modo )
{
    float factor_ancho = pow( float( VID_WIDTH ) / visor_image.cols, ( modo ? 1 : -1 ) );
    float factor_alto = pow( float( VID_HEIGHT ) / visor_image.rows, ( modo ? 1 : -1 ) );

    QPoint point_result;
    point_result.setX( round( point.x() * factor_ancho ) );
    point_result.setY( round( point.y() * factor_alto ) );

    return point_result;
}

QRect MainWindow::convertir_coords( QRect rect, bool modo )
{
    float factor_ancho = pow( float( VID_WIDTH ) / visor_image.cols, ( modo ? 1 : -1 ) );
    float factor_alto = pow( float( VID_HEIGHT ) / visor_image.rows, ( modo ? 1 : -1 ) );

    QRect rect_result;
    rect_result.setX( round( rect.x() * factor_ancho ) );
    rect_result.setY( round( rect.y() * factor_alto ) );
    rect_result.setWidth( round( rect.width() * factor_ancho ) );
    rect_result.setHeight( round( rect.height() * factor_alto ) );

    return rect_result;
}

/* ---     Resolución     --- */

int MainWindow::establecer_resolucion( int index_res )
{
    // Obtenemos la nueva resolución
    VID_WIDTH = resoluciones.at( index_res ).first;
    VID_HEIGHT = resoluciones.at( index_res ).second;

    // Reiniciamos el output y lo configuramos con la nueva resolución
    ::close( output );
    output = open( ( "/dev/video" + to_string( VIDEO_OUT ) ).c_str(), O_WRONLY );

    int status = set_output_format( output, VID_WIDTH, VID_HEIGHT );

    // Establecemos los límites de x, y, ancho y alto de la interfaz
    ui->X_SpinBox->setMaximum( VID_WIDTH - 100 );
    ui->Y_SpinBox->setMaximum( VID_HEIGHT - 100 );
    ui->Altura_SpinBox->setMaximum( VID_HEIGHT );
    ui->Anchura_SpinBox->setMaximum( VID_WIDTH );

    return status;
}

/* ---      Elementos     --- */

int MainWindow::switch_FCN_NET( bool val, int escena, int elemento )
{
    elemento_visual *elto_actual = &lista_eltos_visuales.at( escena ).at( elemento );

    if ( val == true )
    {
        // Inicia el procesamieto de la red fcn
        if ( elto_actual->fcn == nullptr )
        {
            VideoCapture *cam = elto_actual->camara;

            int height = cam->get( CAP_PROP_FRAME_HEIGHT );
            int width = cam->get( CAP_PROP_FRAME_WIDTH );

            fcnmask *fcnd = new fcnmask( width, height );

            // Si hay un error con la FCN devolvemos -1
            if ( fcnd->get_status() == -1 )
                return -1;

            elto_actual->fcn = fcnd;
        }
        elto_actual->thread = new std::thread( &fcnmask::apply_network_FCN, elto_actual->fcn );
    }
    else if ( elto_actual->fcn != nullptr )
    {
        // Detiene el procesamiento de la red
        elto_actual->fcn->stop();
        elto_actual->thread->join();
        delete elto_actual->thread;
        elto_actual->thread = nullptr;
    }
    return 0;
}

void MainWindow::borrar_elto( int escena, int elemento )
{
    elemento_visual *elto_actual = &lista_eltos_visuales.at( escena ).at( elemento );

    if ( elto_actual->is_camara )
    {
        // Borramos el VideoCapture
        elto_actual->camara->release();
        delete elto_actual->camara;
        elto_actual->camara = nullptr;

        // Borramos el threat
        if ( elto_actual->detectar_fondo )
        {
            switch_FCN_NET( false, escena, elemento );
        }

        // Borramos la red FCN
        if ( elto_actual->fcn != nullptr )
        {
            delete elto_actual->fcn;
            elto_actual->fcn = nullptr;
        }
    }

    // Eliminamos elemento de la lista
    lista_eltos_visuales.at( escena ).erase( lista_eltos_visuales.at( escena ).begin() + elemento );
}

/* ---    Procesamiento   --- */

void MainWindow::procesar_elemento_visual( elemento_visual elto )
{
    Mat frame, mascara;

    frame = elto.last_frame.clone();
    /* La mascara es un mat de las mismas dimensiones que el frame con un solo canal, contiene pixeles con valores
     * 255 si el pixel tiene que dibujarse o 0 sino
     */
    mascara = Mat( Size( frame.cols, frame.rows ), CV_8UC1, Scalar( 255 ) );

    /* Si es una cámara con detección de fondo activada entonces:
     * Si está activado el desenfocar fondo lo aplicamos al frame
     * Si lo está el recortar fondo: copiamos la máscara de la red fcn
     * Si es la imagen fondo entonces recortamos usando la mascara y aplicamos una imagen debajo
     */
    if ( elto.is_camara && elto.detectar_fondo )
    {
        Mat mascara_fcn = elto.fcn->get_mask( elto.last_frame );
        if ( elto.tipo_fondo == 0 ) // Recortar Fondo
        {
            mascara = mascara_fcn;
        }
        else if ( elto.tipo_fondo == 1 ) // Desenfocar Fondo
        {
            blur( frame, frame, Size( 25, 25 ) );
            elto.last_frame.copyTo( frame, mascara_fcn );
        }
        else // Imagen Fondo
        {
            frame = elto.imagen_fondo.clone();
            cv::resize( frame, frame, mascara.size(), 0, 0, INTER_NEAREST );
            elto.last_frame.copyTo( frame, mascara_fcn );
        }
    }

    // Aplicamos las modificaciones al frame
    for ( int i = 0; i < ( int )elto.rotaciones.size(); i++ )
    {
        rotacion r = elto.rotaciones[i];

        // Rotación
        if ( r.cantidad != 0 )
        {
            QPointF centro = r.centro;
            if ( elto.orden_proces[2] < i && elto.recorte.isEmpty() == false )
                // Si se ha realizado un recorte previo entonces adaptamos el centro
                centro = QPointF( 0.5, 0.5 );

            frame = rotation_transformation( frame, r.cantidad, centro );
            mascara = rotation_transformation( mascara, r.cantidad, centro );
        }

        // Perspectiva
        if ( elto.orden_proces[0] == i && elto.perspectiva[3] != QPointF() )
        {
            QPointF perspectiva_recorte[4], *perspectiva = elto.perspectiva;
            QRectF region_perspectiva = elto.region_perspectiva;

            if ( elto.orden_proces[2] < i && elto.recorte.isEmpty() == false )
            {
                // Si se ha realizado un recorte previo entonces adaptamos las coordenadas
                transform_to_area_recorte( elto, elto.perspectiva, perspectiva_recorte, 4 );
                perspectiva = perspectiva_recorte;
                region_perspectiva = transform_to_area_recorte( elto, elto.region_perspectiva );
            }

            frame = perspective_transformation( frame, perspectiva, region_perspectiva );
            mascara = perspective_transformation( mascara, perspectiva, region_perspectiva );
        }

        // Pixelación
        if ( elto.orden_proces[1] == i && elto.pixelacion.isEmpty() == false )
        {
            QRectF pixelacion = elto.pixelacion;

            if ( elto.orden_proces[2] < i && elto.recorte.isEmpty() == false )
                pixelacion = transform_to_area_recorte( elto, elto.pixelacion );

            frame = pixelar( frame, pixelacion );
        }

        // Recorte
        if ( elto.orden_proces[2] == i && elto.recorte.isEmpty() == false )
        {
            frame = recortar_video_relativo( frame, elto.recorte );
            mascara = recortar_video_relativo( mascara, elto.recorte );
        }
    }

    // Aplicamos la rotación actual
    frame = rotation_transformation( frame, elto.rotacion_actual, QPointF( 0.50, 0.50 ) );
    mascara = rotation_transformation( mascara, elto.rotacion_actual, QPointF( 0.50, 0.50 ) );

    if ( elto.escala_grises )
    {
        frame = escala_grises( frame );
    }
    else
    {
        frame = control_RGB( frame, elto.R, elto.G, elto.B );
    }

    if ( elto.ecualizado )
    {
        frame = ecualizado( frame );
    }

    if ( elto.filtro_mediana )
    {
        frame = filtro_mediana( frame );
    }

    if ( elto.filtro_afilado )
    {
        frame = filtro_afilado( frame );
    }

    if ( elto.erosionar )
    {
        frame = erisionar( frame );
    }

    // Se reescala el frame y la mascara al tamaño configurado por el usuario
    cv::resize( frame, frame, Size( elto.width, elto.height ) );
    cv::resize( mascara, mascara, Size( elto.width, elto.height ) );

    // Recortamos la parte del elemento que queda fuera de pantalla
    Rect image_rect = Rect( 0, 0, min( elto.width, VID_WIDTH - elto.x ), min( elto.height, VID_HEIGHT - elto.y ) );
    frame = Mat( frame, image_rect );
    mascara = Mat( mascara, image_rect );

    // Se dibuja el frame en processed_image (en las coordenadas configuradas por el usuario) usando la mascara
    image_rect = Rect( elto.x, elto.y, frame.cols, frame.rows );
    frame.copyTo( processed_image( image_rect ), mascara );
}



/// -*-*-*- SIGNALS -*-*-*-

/* ---        Visor       --- */

void MainWindow::select_window_Signal( QRect rect )
{
    selected_rect = rect;

    if ( selected_rect.top() < 0 )
        selected_rect.setTop( 0 );

    if ( selected_rect.left() < 0 )
        selected_rect.setLeft( 0 );

    if ( selected_rect.right() >= visor_image.cols )
        selected_rect.setRight( visor_image.cols - 1 );

    if ( selected_rect.bottom() >= visor_image.rows )
        selected_rect.setBottom( visor_image.rows - 1 );

    win_selected = true;

    // Desactiva el cambio de perspectiva
    ui->Perspectiva_Btn->setChecked( false );
    gestion_interfaz( false );

    // Si alguno de los dos son invalidos entonces descartamos la selección
    if ( selected_rect.width() <= 0 || selected_rect.height() <= 0 )
        win_selected = false;
}

void MainWindow::deselect_window_Signal()
{
    win_selected = false;
}

void MainWindow::window_interact_start_Signal( QPoint point )
{
    // Desactiva la selección de recorte
    win_selected = false;
    tipo_interaccion = 0;

    if ( editando_elto == false )
        return;

    elemento_visual *elto_actual = &lista_eltos_visuales.at( escena_index ).at( elemento_index );

    // Convertimos las coordenadas del punto (del visor) en coordenadas de la imagen de salida
    point = convertir_coords( point, true );

    QPoint inicio_elto = QPoint( elto_actual->x, elto_actual->y );
    QPoint fin_elto = QPoint( elto_actual->x + elto_actual->width, elto_actual->y + elto_actual->height );

    // Definimos las areas de interacción: todo el elto (mover) y la esquina inferior derecha (reescalar)
    QRect elemento = QRect( inicio_elto, fin_elto );
    QRect esquina_inf_der = QRect( fin_elto.x() - 30, fin_elto.y() - 30, 40, 40 );


    // Cambio de Perspectiva
    if ( ui->Perspectiva_Btn->isChecked() && elemento.contains( point ) )
    {
        // Convertimos el punto clicado en coordenadas relativas al elemento (si el punto está dentro) y lo guardamos
        QPointF coords_rel = coords_relativas_elemento( *elto_actual, point, true );
        QPointF *p = elto_actual->perspectiva;
        int punto_actual = p[0] == QPointF() ? 0 : p[1] == QPointF() ? 1 : p[2] == QPointF() ? 2 : 3;
        p[punto_actual] = coords_rel;

        if ( punto_actual != 3 )
            return;

        add_rotacion( elto_actual, 0 );

        // Guardamos la region de la perspectiva (el area de la imagen que será visible y donde se centrará el resultado del cambio de la perspectiva)
        elto_actual->region_perspectiva = QRectF( 0, 0, 1, 1 );
        if ( elto_actual->recorte.isEmpty() == false )
            elto_actual->region_perspectiva = elto_actual->recorte;

        ui->Perspectiva_Btn->setChecked( false );
        gestion_interfaz( false );

        return;
    }


    if ( esquina_inf_der.contains( point ) )
    {
        // Reescalar
        p_com_desp = point - fin_elto + inicio_elto;
        tipo_interaccion = 2;
    }
    else if ( elemento.contains( point ) )
    {
        // Mover
        p_com_desp = point - inicio_elto;
        tipo_interaccion = 1;
    }
}

void MainWindow::window_interact_update_Signal( QPoint point )
{
    if ( editando_elto == false )
        return;

    elemento_visual *elto_actual = &lista_eltos_visuales.at( escena_index ).at( elemento_index );

    // Convertimos las coordenadas del punto (del visor) en coordenadas de la imagen de salida
    point = convertir_coords( point, true );

    // Compensamos el punto donde el usuario ha hecho clic con nuestro punto de interes (esquina superior izquierda al mover por ejemplo)
    point = point - p_com_desp;

    if ( tipo_interaccion == 1 ) // Mover
    {
        if ( point.x() < 0 )
            point.setX( 0 );
        if ( point.y() < 0 )
            point.setY( 0 );
        if ( point.x() > VID_WIDTH - 100 )
            point.setX( VID_WIDTH - 100 );
        if ( point.y() > VID_HEIGHT - 100 )
            point.setY( VID_HEIGHT - 100 );

        elto_actual->x = point.x();
        elto_actual->y = point.y();
    }
    else if ( tipo_interaccion == 2 ) // Reescalar
    {
        if ( point.x() < 100 )
            point.setX( 100 );
        if ( point.y() < 100 )
            point.setY( 100 );
        if ( point.x() > VID_WIDTH )
            point.setX( VID_WIDTH );
        if ( point.y() > VID_HEIGHT )
            point.setY( VID_HEIGHT );

        if ( ui->Aspect_ratio_CheckBox->isChecked() == false )
        {
            elto_actual->width = point.x();
            elto_actual->height = point.y();
        }
        else
        {
            // Hay que mantener el aspecto ratio
            pair<int, int> new_size = adapt_size( elto_actual->aspect_ratio, point.x(), point.y(), true );
            elto_actual->width = new_size.first;
            elto_actual->height = new_size.second;
        }
    }

    mem_to_ui();
}

/* ---   Ventana editar   --- */

void MainWindow::detect_fondo_Btn_Signal( bool val )
{
    if ( switch_FCN_NET( val, escena_index, elemento_index ) == -1 )
    {
        ui->Detectar_Fondo_Btn->setChecked( false );
        error_msg.showMessage( "Falta el archivo 'fcn.caffemodel' dentro de la carpeta 'netData'. Asegurate de tener 'git lfs' antes de hacer 'git clone' o accede a la web del repositorio 'https://github.com/Ivan028/TFG' para descargar el archivo", "fcn_falta_archivo" );
    }
}

void MainWindow::modo_color_Btn_Signal()
{
    elemento_visual *elto_actual = &lista_eltos_visuales.at( escena_index ).at( elemento_index );

    // Intercambiamos el estado de escala_grises
    elto_actual->escala_grises = ! elto_actual->escala_grises;

    gestion_interfaz( false );
}

void MainWindow::modo_fondo_Btn_Signal()
{
    elemento_visual *elto_actual = &lista_eltos_visuales.at( escena_index ).at( elemento_index );

    // Intercambiamos el estado del fondo
    elto_actual->tipo_fondo = ( elto_actual->tipo_fondo + 1 ) % 3;

    gestion_interfaz( false );
}

void MainWindow::recortar_Btn_Signal()
{
    elemento_visual *elto = &lista_eltos_visuales.at( escena_index ).at( elemento_index );

    if ( win_selected == false )
    {
        error_msg.showMessage( "Es necesario seleccionar un area de recorte usando el clic derecho", "selecciona_area" );
        return;
    }

    // Obtenemos la selección y la desactivamos
    QRect rect_abs = win_seleccion_absoluta( *elto );
    QRectF rect_rel = win_seleccion_relativa( *elto );
    win_selected = false;

    if ( rect_abs.isEmpty() || rect_rel.isEmpty() )
        return;

    add_rotacion( elto, 2 );

    // Ponemos las coordenadas al elemento
    elto->x = rect_abs.x() + elto->x;
    elto->y = rect_abs.y() + elto->y;
    elto->width = rect_abs.width();
    elto->height = rect_abs.height();

    elto->recorte = rect_rel;

    mem_to_ui();
    gestion_interfaz( false );
}

void MainWindow::eliminar_recorte_Btn_Signal()
{
    elemento_visual *elto_actual = &lista_eltos_visuales.at( escena_index ).at( elemento_index );

    elto_actual->recorte = QRect();

    // Eliminamos también las otras modificaciones
    elto_actual->pixelacion = QRect();
    elto_actual->perspectiva[3] = QPointF();
    elto_actual->rotaciones.clear();

    elto_actual->rotacion_actual = 0;
    mem_to_ui();

    gestion_interfaz( false );
}

void MainWindow::perspectiva_Btn_Signal( )
{
    elemento_visual *elto_actual = &lista_eltos_visuales.at( escena_index ).at( elemento_index );

    // Restablece la perspectiva
    if ( elto_actual->perspectiva[3] != QPointF() )
    {
        elto_actual->perspectiva[0] = QPointF();
        elto_actual->perspectiva[1] = QPointF();
        elto_actual->perspectiva[2] = QPointF();
        elto_actual->perspectiva[3] = QPointF();
        ui->Perspectiva_Btn->setChecked( false );
    }
    // Inicia el cambio de perspectiva
    else if ( ui->Perspectiva_Btn->isChecked() )
    {
        elto_actual->perspectiva[0] = QPointF();
        elto_actual->perspectiva[1] = QPointF();
        elto_actual->perspectiva[2] = QPointF();
        elto_actual->perspectiva[3] = QPointF();
    }
    gestion_interfaz( false );
}

void MainWindow::aspect_ratio_Checkbox_Signal( bool val )
{
    elemento_visual *elto_actual = &lista_eltos_visuales.at( escena_index ).at( elemento_index );

    if ( val ) // Guardamos el aspecto-ratio
        elto_actual->aspect_ratio = pair<int, int>( elto_actual->width, elto_actual->height );
    else // Lo borramos
        elto_actual->aspect_ratio = pair<int, int>( 0, 0 );
}

void MainWindow::pixelacion_Btn_Signal()
{
    elemento_visual *elto = &lista_eltos_visuales.at( escena_index ).at( elemento_index );

    if ( elto->pixelacion.isEmpty() == false ) // Borra la pixelacion guardada
    {
        elto->pixelacion = QRectF();
    }
    else // Guarda el QRectF para pixelar
    {
        if ( win_selected == false )
        {
            error_msg.showMessage( "Es necesario seleccionar un area para pixelar usando el clic derecho", "selecciona_area" );
            return;
        }

        // Obtenemos la selección y la desactivamos
        QRectF rect = win_seleccion_relativa( *elto );
        win_selected = false;

        if ( rect.isEmpty() )
            return;

        add_rotacion( elto, 1 );

        elto->pixelacion = rect;
    }

    gestion_interfaz( false );
}

void MainWindow::imagen_fondo_Btn_Signal()
{
    try
    {
        QString fileName = QFileDialog::getOpenFileName( this, ( "Open File" ), "/home", ( "Images (*.png *.jpg)" ) );

        if ( fileName.isNull() )
            return;

        elemento_visual *elto = &lista_eltos_visuales.at( escena_index ).at( elemento_index );

        // Cargamos la imagen y guardamos la ruta
        Mat aux = cv::imread( fileName.toStdString().c_str(), IMREAD_COLOR );
        cvtColor( aux, elto->imagen_fondo, COLOR_BGR2RGB );

        elto->imagen_fondo_ruta = fileName;
    }
    catch ( Exception )
    {
        cerr << "Error al cargar la imagen";
    }
}

void MainWindow::salir_Btn_Signal()
{
    // Oculta el panel de editar
    ui->Panel_Editor->setHidden( true );
    ui->Panel_General->setHidden( false );

    // Desactivamos el cambio de perspectiva en proceso (si lo hay)
    ui->Perspectiva_Btn->setChecked( false );

    //Guarda los valores
    editando_elto = false;
    ui_to_mem();
}

/* ---      Elementos     --- */

void MainWindow::elementos_lista_Select_Signal( int val )
{
    // Si el elemento que está seleccionado es inválido hacemos como si no se hubiera seleccionado
    if ( val == -1 )
        return;

    elemento_index = val;
    gestion_interfaz( false );
}

void MainWindow::anadir_elto_Btn_Signal( QAction *q_act )
{
    elemento_visual ev;

    // El primer caracter es el tipo de source y el resto son el id del mismo
    QString tipo = q_act->data().toString().mid( 0, 1 );
    ev.id_source = q_act->data().toString().mid( 1 ).toLongLong();

    // Es un window
    if ( tipo == "W" )
    {
        ev.nombre = QString::fromStdString( get_win_name( display, ev.id_source ) );
        ev.is_camara = false;

        // Establecemos el tamaño del elemento
        pair<int, int> original_size = get_win_size( display, ev.id_source );
        pair<int, int> adapted_size = ( original_size.first == -1 ? make_pair( 1, 1 ) : adapt_size( original_size, VID_WIDTH, VID_HEIGHT, false ) );
        ev.width = adapted_size.first;
        ev.height = adapted_size.second;
    }

    // Es un window especial en el que muestra la pantalla completa
    else if ( tipo == "F" )
    {
        ev.nombre = "Pantalla (" + QString::number( ev.id_source ) + ") completa";
        ev.is_camara = false;

        // La pantalla completa se representa con un número negativo por lo que negamos el número de pantalla
        ev.id_source = - ev.id_source;

        // Establecemos el tamaño del elemento
        pair<int, int> original_size = get_win_size( display, ev.id_source );
        pair<int, int> adapted_size = ( original_size.first == -1 ? make_pair( 1, 1 ) : adapt_size( original_size, VID_WIDTH, VID_HEIGHT, false ) );
        ev.width = adapted_size.first;
        ev.height = adapted_size.second;
    }

    // Es una cámara
    else
    {
        ev.nombre = "Video " + QString::number( ev.id_source );
        ev.is_camara = true;

        // Abrimos la cámara
        ev.camara = new VideoCapture();
        ev.camara->open( ev.id_source );

        // Establecemos el tamaño del elemento
        pair<int, int> original_size = get_cam_size( ev.camara );
        pair<int, int> adapted_size = ( original_size.first == -1 ? make_pair( 1, 1 ) : adapt_size( original_size, VID_WIDTH, VID_HEIGHT, false ) );
        ev.width = adapted_size.first;
        ev.height = adapted_size.second;
    }

    lista_eltos_visuales.at( escena_index ).push_back( ev );

    gestion_interfaz( true );
}

void MainWindow::preparar_anadir_elto_Btn_Signal()
{

    menu_add_elto->clear();

    menu_add_elto->addSection( "Cámaras" );

    // Comprobamos si existe cámaras en "/dev/video"+i desde i=0 hasta i=10, si existe la añadimos a la lista
    VideoCapture cap;
    for ( int i = 0; i <= 10; i++ )
    {
        if ( i == VIDEO_OUT )
            continue;

        cap = VideoCapture( i );
        if ( cap.isOpened() )
        {
            menu_add_elto->addAction( "Video " + QString::number( i ) )->setData( "C" + QString::number( i ) );
            cap.release();
        }
    }

    menu_add_elto->addSeparator();
    menu_add_elto->addSection( "Ventanas" );

    // Añadimos opción de pantalla completa por cada pantalla
    for ( int i = 0; i < get_screens(); i++ )
    {
        menu_add_elto->addAction( "Pantalla (" + QString::number( i ) + ") completa" )->setData( "F" + QString::number( i ) );
    }

    // Añade a la lista todas las ventanas encontradas que tengan un nombre
    unsigned long len;
    char *name;
    Window *list = get_win_list( display, &len );
    if ( list != NULL )
    {
        for ( int i = 0; i < ( int )len; i++ )
        {
            name = get_win_name( display, list[i] );
            if ( name != NULL && ( string )name != "" )
            {
                menu_add_elto->addAction( name )->setData( "W" + QString::number( list[i] ) );
            }
        }
    }

    ui->Anadir_Elto_Btn->setMenu( menu_add_elto );
}

void MainWindow::configurar_elto_Btn_Signal()
{
    elemento_visual *elto_actual = &lista_eltos_visuales.at( escena_index ).at( elemento_index );

    // Habilita/Desabilita botones en función de si es una cámara
    if ( elto_actual->is_camara )
    {
        ui->Detectar_Fondo_Btn->setEnabled( true );
        ui->Modo_Fondo_Btn->setEnabled( true );
    }
    else
    {
        ui->Detectar_Fondo_Btn->setEnabled( false );
        ui->Modo_Fondo_Btn->setEnabled( false );
    }

    // Marcamos el checkbox si el aspecto-ratio estaba bloqueado
    ui->Aspect_ratio_CheckBox->setChecked( elto_actual->aspect_ratio.first != 0 );

    // Carga los valores a la ui
    editando_elto = true;
    mem_to_ui();

    // Muestra el panel de editar
    ui->Panel_Editor->setHidden( false );
    ui->Panel_General->setHidden( true );
    gestion_interfaz( false );
}

void MainWindow::borrar_elto_Btn_Signal()
{
    // Se deselecciona el elemento y lo sacamos de la interfaz
    ui->Elementos_Lista->setCurrentRow( -1 );
    ui->Elementos_Lista->takeItem( elemento_index );

    borrar_elto( escena_index, elemento_index );

    gestion_interfaz( true );
}

/* ---       Escenas      --- */

void MainWindow::escenas_lista_Select_Signal( int val )
{
    // Si la escena que está seleccionada es inválida hacemos como si no se hubiera seleccionado
    if ( val == -1 )
        return;

    int ant_escena = escena_index;
    escena_index = -1;

    // Detiene el VideoCapture y el threat de la antigua escena (si los hay)
    if ( ant_escena != -1 )
    {
        vector<elemento_visual> *escena_actual = &lista_eltos_visuales.at( ant_escena );
        for ( int i = 0; i < ( int )escena_actual->size(); i++ )
        {
            if ( escena_actual->at( i ).is_camara )
            {
                escena_actual->at( i ).camara->release();
                if ( escena_actual->at( i ).detectar_fondo )
                {
                    switch_FCN_NET( false, ant_escena, i );
                }
            }
        }
    }

    // Reanuda los VideoCapture y threats de la nueva escena (si los hay)
    vector<elemento_visual> *escena_actual = &lista_eltos_visuales.at( val );
    for ( int i = 0; i < ( int )escena_actual->size(); i++ )
    {
        if ( escena_actual->at( i ).is_camara )
        {
            escena_actual->at( i ).camara->open( escena_actual->at( i ).id_source );

            // Ponemos el last_frame a negro para evitar que se vea el último frame que se obtuvo cuando estuvo
            // la escena activa por última vez (hasta que la cámara vuelva a estar lista)
            int height = escena_actual->at( i ).camara->get( CAP_PROP_FRAME_HEIGHT );
            int width = escena_actual->at( i ).camara->get( CAP_PROP_FRAME_WIDTH );
            escena_actual->at( i ).last_frame = Mat( Size( width, height ), CV_8UC3, Scalar( 0, 0, 0 ) );

            if ( escena_actual->at( i ).detectar_fondo )
            {
                switch_FCN_NET( true, val, i );
            }
        }
    }

    escena_index = val;
    gestion_interfaz( true );
}

void MainWindow::anadir_escena_Btn_Signal()
{
    // Añadimos un vector de elemento_visual (nuestra escena)
    vector<elemento_visual> nueva;
    lista_eltos_visuales.insert( lista_eltos_visuales.end(), nueva );

    numero_escenas++;
    ui->Escenas_Lista->addItem( "Escena " + QString::number( numero_escenas ) );

    // Si es la única escena la seleccionamos
    if ( lista_eltos_visuales.size() == 1 )
    {
        escena_index = 0;
        ui->Escenas_Lista->setCurrentRow( 0 );
    }

    gestion_interfaz( false );
}

void MainWindow::borrar_escena_Btn_Signal()
{
    // Se deselecciona el elemento y lo sacamos de la interfaz
    ui->Escenas_Lista->setCurrentRow( -1 );
    ui->Escenas_Lista->takeItem( escena_index );

    // Borramos todos los elementos y eliminamos la escena de la lista
    for ( int i = 0; i < ( int )lista_eltos_visuales.at( escena_index ).size(); i++ )
    {
        borrar_elto( escena_index, i );
    }
    lista_eltos_visuales.erase( lista_eltos_visuales.begin() + escena_index );

    escena_index = -1;
    gestion_interfaz( false );
}

void MainWindow::renombrar_escena_Btn_Signal()
{
    ui->Escena_Renombrar->setHidden( false );
    ui->Escena_Botones->setHidden( true );
    ui->Renombrar_Escena_TextEdit->setPlainText( "" );
}

void MainWindow::cancelar_renombrar_escena_Btn_Signal()
{
    ui->Escena_Renombrar->setHidden( true );
    ui->Escena_Botones->setHidden( false );
}

void MainWindow::aceptar_renombrar_escena_Btn_Signal()
{
    QListWidgetItem *escena = ui->Escenas_Lista->currentItem();
    escena->setText( ui->Renombrar_Escena_TextEdit->toPlainText() );
    ui->Escena_Renombrar->setHidden( true );
    ui->Escena_Botones->setHidden( false );
}

void MainWindow::renombrar_Changed_Signal()
{
    QString texto = ui->Renombrar_Escena_TextEdit->toPlainText();
    if ( texto.contains( "\n" ) )
    {
        texto = texto.replace( "\n", "" );
    }
    if ( texto.length() > 30 )
    {
        texto = texto.mid( 0, 30 );
    }
    if ( texto != ui->Renombrar_Escena_TextEdit->toPlainText() )
    {
        ui->Renombrar_Escena_TextEdit->setPlainText( texto );
        QTextCursor cursor = ui->Renombrar_Escena_TextEdit->textCursor();
        cursor.movePosition( QTextCursor::End );
        ui->Renombrar_Escena_TextEdit->setTextCursor( cursor );
    }
}

void MainWindow::exportar_escena_Btn_Signal()
{
    try
    {
        QString escena_text = ui->Escenas_Lista->currentItem()->text();
        QString fileName = QFileDialog::getSaveFileName( this, ( "Save File" ), "/home/" + escena_text + ".scene", "*.scene" );

        if ( fileName.isNull() )
            return;

        // Abrimos el archivo y guardamos el nombre de la escena
        ofstream file;
        file.open( fileName.toStdString(), fstream::trunc );
        file << escena_text.toStdString();

        for ( int i = 0; i < ( int )lista_eltos_visuales.at( escena_index ).size(); i++ )
        {
            elemento_visual elto = lista_eltos_visuales.at( escena_index ).at( i );

            // Obtenemos los valores de algunas propiedades del elemento visual
            stringstream recorte, region_perspectiva, pixelacion, perspectiva, rotaciones;
            qreal q1, q2, q3, q4;

            elto.recorte.getRect( &q1, &q2, &q3, &q4 );
            recorte << q1 << "/" << q2 << "/" << q3 << "/" << q4 ;

            elto.region_perspectiva.getRect( &q1, &q2, &q3, &q4 );
            region_perspectiva << q1 << "/" << q2 << "/" << q3 << "/" << q4 ;

            elto.pixelacion.getRect( &q1, &q2, &q3, &q4 );
            pixelacion << q1 << "/" << q2 << "/" << q3 << "/" << q4 ;

            // Obtenemos los valores de los puntos del cambio de perspectiva para exportarlos
            // exportaremos cada punto como (-1, -1) si no hay cambio de perspectiva
            for ( int i = 0; i < 4; i++ )
            {
                float x = elto.perspectiva[3] == QPointF() ? -1 : elto.perspectiva[i].x();
                float y = elto.perspectiva[3] == QPointF() ? -1 : elto.perspectiva[i].y();
                perspectiva << x << "/" << y;

                if ( i != 3 )
                    perspectiva << "/";
            }

            // Exportamos los elementos del vector rotaciones
            for ( int i = 0; i < ( int )elto.rotaciones.size(); i++ )
            {
                rotacion r = elto.rotaciones.at( i );
                rotaciones << r.centro.x() << "/" << r.centro.y() << "/" << r.cantidad;

                if ( i != ( int )elto.rotaciones.size() - 1 )
                    rotaciones << "#";
            }

            // Escribimos en cada linea un elemento visual
            file << "\n" << elto.x << ";" << elto.y << ";" << elto.width << ";" << elto.height << ";"
                 << elto.nombre.replace( ";", "" ).toStdString() << ";" << elto.is_camara << ";" << elto.id_source << ";" << elto.ocultar << ";"
                 << elto.detectar_fondo << ";" << elto.tipo_fondo << ";" << elto.imagen_fondo_ruta.toStdString() << ";"
                 << elto.escala_grises << ";" << elto.R << ";" << elto.G << ";" << elto.B << ";"
                 << elto.filtro_mediana << ";" << elto.ecualizado << ";" << elto.filtro_afilado << ";" << elto.erosionar << ";"
                 << recorte.str() << ";"
                 << perspectiva.str() << ";" << region_perspectiva.str() << ";"
                 << elto.rotacion_actual << ";" << rotaciones.str() << ";"
                 << elto.orden_proces[0] << "/" << elto.orden_proces[1] << "/" << elto.orden_proces[2] << ";"
                 << pixelacion.str();
        }
        file.close();
    }
    catch ( Exception )
    {
        cerr << "Error al guardar la escena";
        error_msg.showMessage( "Se ha producido un error al exportar la escena", "exportar" );
    }
}

void MainWindow::importar_escena_Btn_Signal()
{
    try
    {
        QString fileName = QFileDialog::getOpenFileName( this, ( "Open File" ), "/home", "*.scene" );

        if ( fileName.isNull() )
            return;

        // Abrimos el archivo y leemos el nombre de la escena
        String str;
        ifstream file;
        file.open( fileName.toStdString() );
        getline( file, str );

        // Añadimos un vector de elemento_visual (nuestra escena)
        lista_eltos_visuales.insert( lista_eltos_visuales.end(), vector<elemento_visual>() );
        numero_escenas++;
        ui->Escenas_Lista->addItem( QString::fromStdString( str ) );

        while ( file.eof() == false )
        {
            // Leemos una linea (un elemento visual) y obtenemos los valores de las variables (separados por ';')
            getline( file, str );
            vector<string> values = split_string( str, ';' );
            elemento_visual elto;

            // A partir de aquí empieza la importación de los datos
            // ----------------------------------------------------

            elto.x = stoi( values[0] );
            elto.y = stoi( values[1] );
            elto.width = stoi( values[2] );
            elto.height = stoi( values[3] );
            elto.nombre = QString::fromStdString( values[4] );
            elto.is_camara = ( values[5] == "1" );
            elto.id_source = stoll( values[6] );
            elto.ocultar = ( values[7] == "1" );
            elto.detectar_fondo = ( values[8] == "1" );
            elto.tipo_fondo = stoi( values[9] );
            elto.imagen_fondo_ruta = QString::fromStdString( values[10] );
            elto.escala_grises = ( values[11] == "1" );
            elto.R = stoi( values[12] );
            elto.G = stoi( values[13] );
            elto.B = stoi( values[14] );
            elto.filtro_mediana = ( values[15] == "1" );
            elto.ecualizado = ( values[16] == "1" );
            elto.filtro_afilado = ( values[17] == "1" );
            elto.erosionar = ( values[18] == "1" );

            // Reemplazamos los '.' por ',' para que el stod haga correctamente la conversión
            // Obtenemos los 4 valores de recorte (separados por '/')
            replace( values[19].begin(), values[19].end(), '.', ',' );
            vector<string> subvalues = split_string( values[19], '/' );
            elto.recorte = QRectF( stod( subvalues[0] ), stod( subvalues[1] ), stod( subvalues[2] ), stod( subvalues[3] ) );

            // Obtenemos los valores de perspectiva (separados por '/'), si son validos los guardamos
            replace( values[20].begin(), values[20].end(), '.', ',' );
            subvalues = split_string( values[20], '/' );
            if ( subvalues[6] != "-1" )
            {
                elto.perspectiva[0] = QPointF( stod( subvalues[0] ), stod( subvalues[1] ) );
                elto.perspectiva[1] = QPointF( stod( subvalues[2] ), stod( subvalues[3] ) );
                elto.perspectiva[2] = QPointF( stod( subvalues[4] ), stod( subvalues[5] ) );
                elto.perspectiva[3] = QPointF( stod( subvalues[6] ), stod( subvalues[7] ) );
            }

            // Obtenemos los 4 valores de region_perspectiva (separados por '/')
            replace( values[21].begin(), values[21].end(), '.', ',' );
            subvalues = split_string( values[21], '/' );
            elto.region_perspectiva = QRectF( stod( subvalues[0] ), stod( subvalues[1] ), stod( subvalues[2] ), stod( subvalues[3] ) );

            elto.rotacion_actual = stoi( values[22] );

            // Obtenemos los valores de rotaciones (cada grupo separados por '#' y los valores por '/')
            replace( values[23].begin(), values[23].end(), '.', ',' );
            subvalues = split_string( values[23], '#' );
            for ( int i = 0; i < ( int )subvalues.size(); i++ )
            {
                vector<string> subsubvalues = split_string( subvalues[i], '/' );

                rotacion r;
                r.centro = QPointF( stod( subsubvalues[0] ), stod( subsubvalues[1] ) );
                r.cantidad = stoi( subsubvalues[2] );
                elto.rotaciones.push_back( r );
            }

            // Obtenemos el orden de procesos (separados por '/')
            subvalues = split_string( values[24], '/' );
            elto.orden_proces[0] = stoi( subvalues[0] );
            elto.orden_proces[1] = stoi( subvalues[1] );
            elto.orden_proces[2] = stoi( subvalues[2] );

            // Obtenemos los 4 valores de pixelación (separados por '/')
            replace( values[25].begin(), values[25].end(), '.', ',' );
            subvalues = split_string( values[25], '/' );
            elto.pixelacion = QRectF( stod( subvalues[0] ), stod( subvalues[1] ), stod( subvalues[2] ), stod( subvalues[3] ) );

            // --------------------------------------------
            // Aquí acaba la importación de los datos

            // Hacemos unas últimas configuraciones
            if ( elto.is_camara )
                elto.camara = new VideoCapture();

            if ( elto.imagen_fondo_ruta != "" )
            {
                // Cargamos la imagen
                Mat aux = cv::imread( elto.imagen_fondo_ruta.toStdString().c_str(), IMREAD_COLOR );

                // Si la imagen se ha podido cargar entonces la guarda
                if ( aux.data )
                    cvtColor( aux, elto.imagen_fondo, COLOR_BGR2RGB );
            }

            lista_eltos_visuales.at( lista_eltos_visuales.size() - 1 ).push_back( elto );
        }

        file.close();
        gestion_interfaz( false );
    }
    catch ( Exception )
    {
        cerr << "Error al cargar la escena";
        error_msg.showMessage( "Se ha producido un error al importar la escena, puede que el resultado sea una escena vacía o con algún elemento faltante", "importar" );
    }
}

/* --- Acciones elementos --- */

void MainWindow::flecha_arriba_Btn_Signal()
{
    vector<elemento_visual> *escena_actual = &lista_eltos_visuales.at( escena_index );

    // Intercambiamos el elemento actual y el anterior
    swap( escena_actual->at( elemento_index ), escena_actual->at( elemento_index - 1 ) );

    gestion_interfaz( true, elemento_index - 1 );
}

void MainWindow::ocultar_elto_Btn_Signal()
{
    elemento_visual *elto_actual = &lista_eltos_visuales.at( escena_index ).at( elemento_index );

    // Intercambiamos el estado del fondo
    elto_actual->ocultar = ! elto_actual->ocultar;

    gestion_interfaz( true, elemento_index );
}

void MainWindow::flecha_abajo_Btn_Signal()
{
    vector<elemento_visual> *escena_actual = &lista_eltos_visuales.at( escena_index );

    // Intercambiamos el elemento actual y el siguiente
    swap( escena_actual->at( elemento_index ), escena_actual->at( elemento_index + 1 ) );

    gestion_interfaz( true, elemento_index + 1 );
}

/* --- Acciones generales --- */

void MainWindow::pausa_Btn_Signal( bool val )
{
    pausar = val;
}

void MainWindow::framerate_Signal( int val )
{
    timer.stop();
    timer.start( 1.0 / val * 1000 );
}

void MainWindow::resolucion_Signal( int val )
{
    // Guardamos la resolución actual
    int wd_ant = VID_WIDTH;
    int hg_ant = VID_HEIGHT;

    // Establecemos la resolución
    int status = establecer_resolucion( val );

    // Si no se ha podido configurar entonces algún programa mantiene el output abierto, deshacemos los cambios
    if ( status == -4 )
    {
        VID_WIDTH = wd_ant;
        VID_HEIGHT = hg_ant;
        cerr << "Para cambiar la resolución cierra primero cualquier programa que esté usando el vídeo" << "\n";
        error_msg.showMessage( "Para cambiar la resolución cierra primero cualquier programa que esté usando el vídeo", "cambiar_resolucion" );

        for ( int i = 0; i < ( int ) resoluciones.size(); i++ )
        {
            if ( wd_ant == resoluciones.at( i ).first && hg_ant == resoluciones.at( i ).second )
            {
                ui->Resolucion_ComboBox->setCurrentIndex( i );
                break;
            }
        }
        return;
    }

    // Reescalamos los elementos visuales
    if ( escena_index != -1 )
    {
        vector<elemento_visual> *escena = &lista_eltos_visuales.at( escena_index );
        for ( int i = 0; i < ( int )escena->size(); i++ )
        {
            escena->at( i ).x = round( escena->at( i ).x * VID_WIDTH / wd_ant );
            escena->at( i ).y = round( escena->at( i ).y * VID_HEIGHT / hg_ant );
            escena->at( i ).width = round( escena->at( i ).width * VID_WIDTH / wd_ant );
            escena->at( i ).height = round( escena->at( i ).height * VID_HEIGHT / hg_ant );
        }
    }
}

/* ---    Procesamiento   --- */

void MainWindow::compute_Signal()
{
    if ( editando_elto )
    {
        ui_to_mem();

        // Dibujamos un rectángulo rojo alrededor del elemento que se está editando
        elemento_visual *elto_actual = &lista_eltos_visuales.at( escena_index ).at( elemento_index );
        QRect area_elto = QRect( elto_actual->x, elto_actual->y, elto_actual->width, elto_actual->height );
        visor->dibujar_cuadrado( convertir_coords( area_elto, false ), Qt::red );
    }

    processed_image = Mat( Size( VID_WIDTH, VID_HEIGHT ), CV_8UC3, Scalar( 0, 0, 0 ) );

    // Procesamos los elementos visuales
    if ( escena_index != -1 )
    {
        vector<elemento_visual> *escena = &lista_eltos_visuales.at( escena_index );
        for ( int i = escena->size() - 1; i >= 0; i-- )
        {
            elemento_visual *elto = &escena->at( i );

            if ( elto->ocultar )
                continue;

            bool elto_disponible;

            if ( elto->is_camara )
                elto_disponible = get_cam_frame( elto->camara, &elto->last_frame );
            else
                elto_disponible = get_screen_frame( display, elto->id_source, &elto->last_frame );

            if ( elto_disponible )
                procesar_elemento_visual( *elto );
        }
    }

    if ( win_resize )
    {
        win_resize = false;
        reescalar_visor();
    }

    // Mostramos la imagen en el visor
    cv::resize( processed_image, visor_image, Size( visor_image.cols, visor_image.rows ) );

    // Escribimos en el flujo de salida
    if ( pausar == false )
    {
        cvtColor( processed_image, output_image, COLOR_RGB2YUV_I420 );
        if ( ui->Invertir_Salida_Btn->isChecked() )
            flip( output_image, output_image, 1 );
    }
    write( output, output_image.data, VID_WIDTH * VID_HEIGHT * 1.5 );

    // Mostramos la selección de recorte
    if ( win_selected )
        visor->dibujar_cuadrado( selected_rect, Qt::green );

    // Mostramos los puntos del cambio de perspectiva
    if ( ui->Perspectiva_Btn->isChecked() )
    {
        elemento_visual *elto_actual = &lista_eltos_visuales.at( escena_index ).at( elemento_index );

        for ( int i = 0; i < 4; i++ )
        {
            if ( elto_actual->perspectiva[i] != QPointF() )
            {
                QPointF coords_abs = coords_relativas_elemento( *elto_actual, elto_actual->perspectiva[i], false );
                // Convertimos las coordenadas del punto (de la imagen de salida) en coordenadas del visor
                QPoint point = convertir_coords( coords_abs.toPoint(), false );
                visor->dibujar_circulo( point, 4, QColor( 255, 204, 0 ) ); //Color naranja
            }
        }
    }

    visor->update();
}



MainWindow::~MainWindow()
{
    XCloseDisplay( display );
    ::close( output );

    delete ui;
    delete visor;
    delete menu_add_elto;

    visor_image.release();
    processed_image.release();
    output_image.release();

    for ( int i = 0; i < ( int ) lista_eltos_visuales.size(); i++ )
    {
        for ( int j = 0; j < ( int ) lista_eltos_visuales.at( i ).size(); j++ )
        {
            borrar_elto( i, j );
        }
    }
}
