# Polo Points
Programa para ESP32 que administra un tablero de LEDs deportivo pensado para Polo. La comunicación con este es serial RS232, a través de una placa controladora del tablero de LEDs.
La app permite administrar puntajes local y visitante, chukker y temporizador. El dispositivo ESP32 implementa además un servidor WiFi con modalidad access point que administra distintas consultas para interactuar con el tablero. Se ofrece también un front-end sencillo (archivo html) en la dirección del host para probar la funcionalidad del programa. Los datos de la red son los siguientes:
* IP: 192.168.1.5
* Dominio: **polopoints** &rarr; posibilidad de acceder a través de servidor DNS
* SSID: Polo Points
* PASS: 12345678

Es posible incrementar/decrementar puntajes (local y visitante) y chuker.

El reloj puede iniciarse, frenarse o resetarse, como así también modificar su valor actual o el default. Programado inicialmente para iniciar los 6:30 minutos de cada chukker y los 3 minutos de descanso entre chukkers.

Es posible reiniciar todo el tablero a sus valores default.

## Instalación
Proyecto desarrollado inicialmente en el IDE de Arduino y luego migrado a Visual Studio Code, mediante la extensión Platform IO.
Es necesario instalar la herramienta ESP-IDF para poder agregar la dependencia **mDNS** (servidos DNS).
* [esp32/api-reference/protocols/mdns](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/protocols/mdns.html)
* [Instalación ESP-IDF](https://github.com/espressif/vscode-esp-idf-extension/blob/master/docs/tutorial/install.md)

Ejecutar desde una terminal ESP-IDF (accesible desde VSCode):
```
idf.py add-dependency espressif/mdns
```