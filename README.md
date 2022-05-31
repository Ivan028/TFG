# TFG
## Pre-requisitos
Es necesario tener instalado [v4l2loopback](https://github.com/umlaeute/v4l2loopback)
```
git clone https://github.com/umlaeute/v4l2loopback
```
En el directorio donde lo hayamos descargado:
```
make
make clean
make && sudo make install
sudo depmod -a
```
Una vez instalado creamos el dispositivo de vídeo
```
modprobe v4l2loopback video_nr=12
```
También hay que pasar el archivo [fcn.caffemodel](https://drive.google.com/file/d/19fnZawnduOtk0KjVv_tzdTA72C5F0_4o/view?usp=sharing) a la carpeta _netData_
## Consideraciones
* La **resolución de salida** solo cambiará si no hay ningún programa haciendo uso del vídeo.
* Los **botones de "Pausar" e "Invertir salida"** tiene efecto sobre el vídeo de salida y no sobre el visor (el cual no mostrará ningún cambio)
* Las **flechas a la derecha de la lista de elementos** moverán el elemento seleccionado a una capa superior/inferior, causando que esté por encima/debajo de otros elementos.
* El **botón entre las flechas** ocultará/mostrará el elemento, el nombre del elemento se verá tachado (indicando que está oculto) o no.
* Al **rotar un elemento** entre 60 y 120 grados la escala se reducirá para que entre verticalmente.
* Con el **clic izquierdo** del ratón se podrá, mientras se esté en la pantalla de editar elemento, mover el elemento y reescalarle (pinchando en la esquina inferior derecha del mismo)
* Con el **clic derecho** se podrá seleccionar un área en el visor necesaria para poder recortar.
