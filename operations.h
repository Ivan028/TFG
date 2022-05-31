#ifndef OPERATIONS_H
#define OPERATIONS_H

#include <opencv2/opencv.hpp>

#include <QtWidgets>

#include <sys/ioctl.h>
#include <linux/videodev2.h>

using namespace cv;
using namespace std;

#ifndef WRAPPER_HPP
#define WRAPPER_HPP

#include <X11/Xlib.h>

#undef None
constexpr auto None = 0L;

#undef KeyPress
constexpr auto KeyPress = 2;

#undef KeyRelease
constexpr auto KeyRelease = 2;

#undef FocusIn
constexpr auto FocusIn = 9;

#undef FocusOut
constexpr auto FocusOut = 10;

#undef FontChange
constexpr auto FontChange = 255;

#undef Expose
constexpr auto Expose = 12;

#undef False
constexpr auto False = 0;

#undef True
constexpr auto True = 1;

#undef Status
typedef int Status;

#undef Unsorted
constexpr auto Unsorted = 0;

#undef Bool
typedef int Bool;

#undef CursorShape
constexpr auto CursorShape = 0;

#endif // WRAPPER_HPP

#include <X11/Xutil.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>


/* Devuelve img convertido en escala de grises (en rgb)
 */
Mat escala_grises( Mat img );

/* Devuelve un Mat resultante de la intersección de img y recorte
 */
Mat recortar_video( Mat img, Rect recorte );

/* Devuelve un Mat resultante de la intersección de img y recorte
 * las coordenadas de recorte son relativas (en porcentaje)
 */
Mat recortar_video_relativo( Mat img, QRectF recorte );

/* Devuelve img tras realizar un ecualizado
 */
Mat ecualizado( Mat img );

/* Devuelve img tras realizar un filtro mediana
 */
Mat filtro_mediana( Mat img );

/* Aplica a "img" una rotación  de "val" grados, si la imagen se está rotando entre 60 y 120 grados la escala de la imagen
 * se reducirá para que quepa verticalmente.
 */
Mat rotation_transformation( Mat img, int val );

/* Aplica a "img" una transformación de perspectiva, inclinará la parte superior de la imagen
 * "val" especifica de cuanto será esa transformación (en porcentaje)
 */
Mat perspective_transformation( Mat img, int val );

/* Devuelve img modificando el canal de cada color.
 * Si el valor es < 100 dicho canal se verá atenuado. Por ejemplo si es 70 el canal se atenuará un 30%
 * Si es > 100 se verá incrementado. Por ejemplo un valor de 120 incrementará un 20% dicho canal
 * El valor por defecto es 100
 */
Mat control_RGB( Mat img, int red, int green, int blue );

/* Devuelve una lista de Window con las ventanas que hay abiertas
 * len es un parametro de salida, contiene la longitud de la lista
 * Devolverá NULL si no se ha podido obtener
 */
Window *get_win_list( Display *disp, unsigned long *len );

/* Devuelve el nombre de la ventana pasada como parámetro
 * Devolverá NULL si no se ha podido obtener
 */
char *get_win_name( Display *disp, Window win );

/* Si las dimensiones en "size" no son mayores que "limit_width" y "limit_height" entonces devuelve "size" (ancho, alto)
 * Si no, devuelve las dimensiones adaptadas a partir de las proporcionadas en "size", las nuevas dimensiones
 * tendrá la misma proporción pero reducidas para que sean menores que "limit_width", "limit_height"
 */
pair<int, int> adapt_size( pair<int, int> size, int limit_width, int limit_height );

/* Devuelve un par de enteros con las dimensiones ancho y alto (por ese orden) de win
 * Si la ventana no existe devolverá un par de enteros -1, -1.
 * win también puede tomar valores negativos para identificar la pantalla completa
 * win contendrá el id de la pantalla negado, esto es, para la pantalla 1 valdrá -1
 */
pair<int, int> get_win_size( Display *disp, long long win );

/* Devuelve un par de enteros con las dimensiones ancho y alto (por ese orden) de cam
 * Si la cámara no está abierta devolverá un par de enteros -1, -1
 */
pair<int, int> get_cam_size( VideoCapture *cam );

/* Devuelve el número de pantallas que hay
 */
int get_screens();

/* Devuelve true si la camara está abierta o false si no lo está
 * Pone en *image el frame obtenido de la cámara (si había uno listo) o no pondrá nada en el caso
 * de que la cámara aún no esté lista
 */
bool get_cam_frame( VideoCapture *cam, Mat *image );

/* Devuelve true si la ventana existe o false en caso contrario
 * Pone en *image el frame obtenido de la captura de la ventana o no pondrá nada en el caso de
 * que la ventana no exista.
 * Si win es negativo o 0 es una captura de la pantalla completa.
 * win contendrá el id de la pantalla negado, esto es, para la pantalla 1 valdrá -1
 */
bool get_screen_frame( Display *disp, long long win, Mat *image );

/* Establece el formato para el output con altura "height", anchura "width" y formato de pixel "YUV420"
 * Devolverá 0 en caso de exito. Todos los errores menos el -4 escribirán datos del error en "cerr"
 * -1 si el output no está abierto
 * -2 si no se puede obtener el formato de video del output
 * -3 si no se pudo establecer el formato
 * -4 si la anchura y/o altura del output no es la especificada
 */
int set_output_format( int output, int width, int height );


#endif // OPERATIONS_H
