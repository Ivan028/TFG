#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <operations.h>
#include <fcnmask.h>

#include <imageviewer.h>

#include <QMainWindow>
#include <QTimer>
#include <opencv2/opencv.hpp>
#include <thread>
#include <QMenu>
#include <unistd.h>
#include <string>

#include <fstream>

using namespace cv;
using namespace std;

QT_BEGIN_NAMESPACE

namespace Ui
{
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow( QWidget *parent = nullptr );
    ~MainWindow();

private:
    Ui::MainWindow *ui;

    QTimer timer;

    Mat visor_image, output_image, processed_image;

    // Contiene el descriptor de fichero "/dev/video"+i donde se escriben los frames
    int output;

    /* --- Visor --- */
    imageviewer *visor;

    // "win_selected" será True si hay un area seleccionada en el visor (el area estará almacenada en "selected_rect"),
    // false en caso contrario
    bool win_selected;
    QRect selected_rect;

    // "resizeEvent" será llamado cuando la ventana cambie de tamaño y win_resize se pondrá en True para indicar
    // que hay que reescalar el visor
    bool win_resize;
    void resizeEvent( QResizeEvent* event );

    // "tipo_interaccion" es usado para saber que acción hacer cuando "window_interact_update_Signal" sea llamado.
    // 0 no hacer nada, 1 mover la ventana, 2 reescalarla. "p_com_desp" es usado para compensar el desplazamiento
    int tipo_interaccion;
    QPoint p_com_desp;

    // Cambia el visor de tamaño para que siempre esté centrado y ocupe el máximo de la dimensión más pequeña
    void reescalar_visor();

    // Convierte las coordenadas pasadas, del visor a la imagen de salida (si "modo" = true)
    // o de la imagen de salida al visor ("modo" = false)
    QPoint convertir_coords( QPoint point, bool modo );
    QRect convertir_coords( QRect rect, bool modo );

    Display *display;

    int numero_escenas = 0;
    QMenu *menu_add_elto;

    bool pausar = false;

    /* --- Resolución --- */
    // Ancho y alto de la salida de vídeo
    int VID_WIDTH;
    int VID_HEIGHT;

    // Lista de resoluciones
    vector<pair<int, int>> resoluciones { {960, 540}, {1280, 720}, {1920, 1080} };

    // Establece la resolución de salida y configura el output de acuerdo con la resolución seleccionada
    // en index_res hay que especificar el index del elemento del vector "resoluciones"
    // Devolverá el valor resultante de la llamada a "set_output_format"
    int establecer_resolucion( int index_res );

    struct elemento_visual
    {
        int x = 0;
        int y = 0;
        int width = 1;
        int height = 1;
        QString nombre;
        bool is_camara;
        VideoCapture *camara = nullptr;
        fcnmask *fcn = nullptr;
        std::thread *thread = nullptr;
        // Es un long long para poder representar todos los valores de window (u long) y números negativos (pantalla completa)
        long long id_source = 0;
        Mat last_frame = Mat( Size( 1, 1 ), CV_8UC3, Scalar( 0, 0, 0 ) );
        bool ocultar = false;
        bool detectar_fondo = false;
        bool fondo_borroso = true;
        bool escala_grises = false;
        int R = 100;
        int G = 100;
        int B = 100;
        bool filtro_mediana = false;
        bool ecualizado = false;
        bool filtro_afilado = false;
        bool erisionar = false;
        QRectF recorte;
        QPointF perspectiva[4];
        QRectF region_perspectiva;
        int rotacion = 0;
        QRectF pixelacion;
        pair<int, int> aspect_ratio = pair<int, int>( 0, 0 );
    };

    vector<vector<elemento_visual>> lista_eltos_visuales;

    // Indica el index de la escena seleccionada, -1 si no hay ninguna
    int escena_index = -1;
    // Indica el index del elemento que está seleccionado, -1 si no se está editando ninguno
    int elemento_index = -1;

    // Guarda en "processed_image" el elemento "elto" después de aplicar a "last_frame" las modificaciones especificadas
    // en el elemento
    void procesar_elemento_visual( elemento_visual elto );

    // "editando_elto" será True cuando se esté en la ventana de editar un elemento
    // La función "mem_to_ui" carga los valores del elemento_visual a la interfaz. "ui_to_mem" realiza el proceso contrario
    bool editando_elto = false;
    void mem_to_ui();
    void ui_to_mem();

    // Devuelve un vector de strings tras cortar el texto en cada delimitador
    vector<string> split_string( string texto, char delimitador );

    // Convierte las coordenadas pasadas a coordenadas relativas con respecto al elemento (si "modo" = true)
    // o a cordenadas absolutas a partir de las coordenadas relativas al elemento ("modo" = false)
    QPointF coords_relativas_elemento( elemento_visual elto, QPointF point, bool modo );

    // Devuelve un QRect que son las coordenadas absolutas de la selección (respecto del elemento)
    QRect win_seleccion_absoluta( elemento_visual elto );

    // Devuelve un QRect que son las coordenadas relativas (en porcentaje) de la selección (respecto del elemento)
    QRectF win_seleccion_relativa( elemento_visual elto );

    /* --- Interfaz --- */
    // Se encarga de gestionar la interfaz referente a la sección de editar y a la lista de elementos
    void gestion_interfaz_seccion_editar();
    void gestion_interfaz_elto();

    // Gestiona la interfaz relacionada con la lista de escenas, "update_lista_eltos" a true (si hay una escena seleccionada)
    // borrará la lista de elementos (en la interfaz) y la creará de nuevo, después seleccionará el elto con index
    // "seleccionar", si no se especifica entonces no se seleccionará ninguno
    void gestion_interfaz_escena( bool update_lista_eltos, int seleccionar );

    // Llama al gestionar correspondiente
    void gestion_interfaz( bool update_lista_eltos, int seleccionar );

    // Inicia/detiene (dependiendo de val) el procesamiento de la red fcn en el elemento y escena determinados
    void switch_FCN_NET( bool val, int escena, int elemento );

    // Borra el elemento especificado de "lista_eltos_visuales"
    void borrar_elto( int escena, int elemento );

public slots:
    void compute_Signal();

    /* --- Visor --- */
    void select_window_Signal( QRect rect );
    void deselect_window_Signal();
    void window_interact_start_Signal( QPoint point );
    void window_interact_update_Signal( QPoint point );

    /* --- Ventana editar --- */
    void modo_color_Btn_Signal();
    void detect_fondo_Btn_Signal( bool val );
    void modo_fondo_Btn_Signal();
    void recortar_Btn_Signal();
    void eliminar_recorte_Btn_Signal();
    void perspectiva_Btn_Signal( );
    void aspect_ratio_Checkbox_Signal( bool val );
    void pixelacion_Btn_Signal();
    void salir_Btn_Signal();

    /* --- Elementos --- */
    void elementos_lista_Select_Signal( int val );
    void anadir_elto_Btn_Signal( QAction *q_act );
    void preparar_anadir_elto_Btn_Signal();
    void configurar_elto_Btn_Signal();
    void borrar_elto_Btn_Signal();

    /* --- Escenas --- */
    void escenas_lista_Select_Signal( int val );
    void anadir_escena_Btn_Signal();
    void borrar_escena_Btn_Signal();
    void renombrar_escena_Btn_Signal();
    void cancelar_renombrar_escena_Btn_Signal();
    void aceptar_renombrar_escena_Btn_Signal();
    void renombrar_Changed_Signal();
    void exportar_escena_Btn_Signal();
    void importar_escena_Btn_Signal();

    /* --- Acciones elementos --- */
    void flecha_arriba_Btn_Signal();
    void ocultar_elto_Btn_Signal();
    void flecha_abajo_Btn_Signal();

    /* --- Acciones generales --- */
    void pausa_Btn_Signal( bool val );
    void framerate_Signal( int val );
    void resolucion_Signal( int val );
};
#endif // MAINWINDOW_H
