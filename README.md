# telebot
C Library for Telegram bot API that uses json-c and libcurl

## Compilation
You need to install libraries and build tools such as CMake. On Debian-base Linux distributions you can do it as follows:
```sh
sudo apt-get install libcurl3-dev libjson0 libjson0-dev cmake binutils make
```
To compile the library execute following commands:
```sh
cd <your repository>
mkdir -p Build && cd Build
cmake ../
make 
```
