# TFG - Herramienta de transformación de vídeo para aplicaciones de vídeoconferencia
## Pre-requisitos

### fcn.caffemodel

Hay que pasar el archivo [fcn.caffemodel](https://drive.google.com/file/d/19fnZawnduOtk0KjVv_tzdTA72C5F0_4o/view?usp=sharing) a la carpeta _netData_.

Si lfs está instalado al hacer 'git clone' se descaragará automaticamente, si se ha superado la cuota de transferencia de GitHub entonces esto no sucederá y habrá que hacerlo manualmente. En el caso de que este archivo falte se mostrará un error al usuario de dicha falta al activar la opción 'Detectar fondo'.

### v4l2loopback

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
### Otras dependencias
* Opencv 4.2+
* Qt 5.14+

## Compilar y ejecutar

En el directorio donde está el código haremos:
```
qmake .
make
./TFG
```

## Consideraciones
* La **resolución de salida** solo cambiará si no hay ningún programa haciendo uso del vídeo (si fuera el caso el usuario sería avisado).
* Los **botones de "Pausar" e "Invertir salida"** tiene efecto sobre el vídeo de salida y no sobre el visor (el cual no mostrará ningún cambio)
* Las **flechas a la derecha de la lista de elementos** moverán el elemento seleccionado a una capa superior/inferior, causando que esté por encima/debajo de otros elementos.
* El **botón entre las flechas** ocultará/mostrará el elemento, el nombre del elemento se verá tachado (indicando que está oculto) o no.
* Al **rotar un elemento** entre 60 y 120 grados la escala se reducirá para que entre verticalmente.
* Con el **clic izquierdo** del ratón se podrá, mientras se esté en la pantalla de editar elemento, mover el elemento y reescalarle (pinchando y moviendo en la esquina inferior derecha del mismo), el checkbox de "Bloquear aspect-ratio" funciona sobre el reescalado con ratón, no sobre las dimensiones de la textbox (Dicha opción se mantiene como la dejas, se almacena el estado pero no se exportará).
* Con el **clic derecho** se podrá seleccionar un área en el visor, necesaria para poder recortar y pixelar; es necesaria tenerla seleccionada previamente.
* Tras haber realizado al menos un recorte aparecerá un botón con **una 'X'** al lado para **borrar los cambios hechos por el recorte**, el rotar, pixelar y de perspectiva
* Cuando se selecciona la opción de **'Imagen fondo'** aparece un botón al lado con un **signo '+' con el que poder elegir una imagen** (pudiéndola cambiar en cualquier momento usando de nuevo el botón)
* Para **cambiar la perspectiva** hay que seleccionar 4 puntos después de presionar el botón, que serán los usados para realizar el cambio (usando el clic izquierdo). Se puede detener el cambio presionando de nuevo el botón (en el cual pondrá "cancelar")
