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
modprobe v4l2loopback video_nr=5
```
