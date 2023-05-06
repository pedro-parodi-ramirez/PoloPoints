# Polo Points
Programa para ESP32 que administra un tablero de LEDs deportivo pensado para Polo. La comunicación con este es serial RS232, a través de una placa controladora del tablero de LEDs.
La app permite administrar puntajes local y visitante, chukker y temporizador. El dispositivo ESP32 implementa además un servidor WiFi con modalidad access point que administra distintas consultas para interactuar con el tablero. Se ofrece también un front-end sencillo (archivo html) en la dirección del host para probar la funcionalidad del programa. Los datos de la red son los siguientes:
* IP: 192.168.4.1
* SSID: Polo Points
* PASS: 12345678

Es posible incrementar/decrementar puntajes (local y visitante) y chuker.

El reloj puede iniciarse, frenarse o resetarse, como así también modificar su valor actual o el default. Programado inicialmente para iniciar los 6:30 minutos de cada chukker y los 3 minutos de descanso entre chukkers.

Es posible reiniciar todo el tablero a sus valores default.

No está implementada de momento la chicharra o campana sonora que acompaña a eventos especificos del timer.