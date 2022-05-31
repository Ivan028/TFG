#include "imageviewer.h"

imageviewer::imageviewer( QWidget *parent, Mat *img ) : QGLWidget( parent )
{
    if ( img->empty() == false )
    {
        set_mat( img );
    }
    else
    {
        width = parent->width();
        height = parent->height();
    }

    show();
}

imageviewer::~imageviewer()
{
    if ( q_img != nullptr )
        delete q_img;
}


void imageviewer::mat_to_qimage( Mat src, QImage *dest, int height, int linesize )
{
    for ( int i = 0; i < height; i++ )
    {
        memcpy( dest->scanLine( i ), src.ptr( i ), linesize );
    }
}

void imageviewer::set_mat( Mat *img )
{
    mat_img = img;
    width = img->cols;
    height = img->rows;
    q_img = new QImage( width, height, QImage::Format_RGB888 );

    resize( parentWidget()->width(), parentWidget()->height() );
    area_viewer.setRect( 0, 0, parentWidget()->width(), parentWidget()->height() );
}


void imageviewer::set_image( Mat *img )
{
    if ( img->empty() )
        return;

    if ( q_img != nullptr )
        delete q_img;

    set_mat( img );
}

void imageviewer::set_position( int x, int y )
{
    pos_X = x;
    pos_Y = y;
}

void imageviewer::paintEvent( QPaintEvent * )
{
    QPainter painter( this );
    painter.setRenderHint( QPainter::HighQualityAntialiasing );

    painter.setWindow( area_viewer );

    if ( mat_img != nullptr && mat_img->empty() == false && mat_img->type() == CV_8UC3 )
        mat_to_qimage( *mat_img, q_img, height, width * 3 * sizeof( uchar ) );

    // Dibujamos la imagen
    if ( q_img != nullptr )
        painter.drawImage( pos_X, pos_Y, *q_img );

    if ( seleccionando_window )
        dibujar_cuadrado( QRect( start_coords_selected, end_coords_selected ), Qt::green );

    // Se dibujan los cuadrados
    QPen pen = painter.pen();
    while ( cola_cuadrados.empty() == false )
    {
        cuadrado c = cola_cuadrados.front();
        cola_cuadrados.pop();
        pen.setColor( c.color );
        painter.setPen( pen );
        painter.setBrush( Qt::transparent );
        painter.drawRect( c.rect );
    }
}

void imageviewer::dibujar_cuadrado( QRect rect, QColor color )
{
    cuadrado c;
    c.rect = QRect( rect.x() + pos_X, rect.y() + pos_Y, rect.width(), rect.height() );
    c.color = color;
    cola_cuadrados.push( c );
}


void imageviewer::mousePressEvent( QMouseEvent *event )
{
    if ( event->button() == Qt::LeftButton )
    {
        seleccionando_interact = true;
        emit win_interact_start( QPoint( event->x() - pos_X, event->y() - pos_Y ) );
    }
    else if ( event->button() == Qt::RightButton )
    {
        start_coords_selected.setX( event->x() - pos_X );
        start_coords_selected.setY( event->y() - pos_Y );
        end_coords_selected.setX( event->x() - pos_X );
        end_coords_selected.setY( event->y() - pos_Y );

        seleccionando_window = true;
        emit windowCancel();
    }
}

void imageviewer::mouseMoveEvent( QMouseEvent *event )
{
    if ( seleccionando_interact )
    {
        emit win_interact_update( QPoint( event->x() - pos_X, event->y() - pos_Y ) );
    }
    else if ( seleccionando_window )
    {
        end_coords_selected.setX( event->x() - pos_X );
        end_coords_selected.setY( event->y() - pos_Y );
    }
}

void imageviewer::mouseReleaseEvent( QMouseEvent *event )
{
    if ( event->button() == Qt::LeftButton )
    {
        seleccionando_interact = false;
    }
    else if ( event->button() == Qt::RightButton )
    {
        int x = min( start_coords_selected.x(), end_coords_selected.x() );
        int y = min( start_coords_selected.y(), end_coords_selected.y() );
        int width = max( start_coords_selected.x(), end_coords_selected.x() ) - x;
        int height = max( start_coords_selected.y(), end_coords_selected.y() ) - y;

        emit windowSelected( QRect( x, y, width, height ) );
        seleccionando_window = false;
    }
}
