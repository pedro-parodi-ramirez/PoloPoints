# TO-DO
* Emprolijar el llamado a funcion refreshScoreboard(). Parece realizar muchas tareas.
* Emprolijar la forma en que front-end consulta los datos de tablero a ESP32 (request cada 200ms). Agustin propone un request y se frena hasta actualización de algún valor y responde, o bien espera actualización máximo X segundos.
* html para tablet 6'' Lenovo Ideatab A1000l-f
* Seleccionar parlates (plastico tipo alarma, 2u 20W c/u), amp. audio, DC-DC convertidor (12V a 5V).

# WIFI
Programa funcionando que acepta comandos a traves de un sitio-web alojado en ESP32.
IP fija: 192.168.4.1
SSID: PoloPoints
PASS: 12345678
Es posible: incrementar/decrementar puntajes (local y visitante) y chuker. El timer puede iniciarse, frenarse o resetarse, como así también modificar su valor actual o el default. Es posible reiniciar todo el tablero a sus valores default.
No está implementada la chicharra o campana sonora que acompaña a eventos especificos del timer.