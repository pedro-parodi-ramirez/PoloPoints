#SERIAL

Programa funcionando que acepta comandos por serial (UART).
Es posible: incrementar/decrementar puntajes (team 1 y 2) y chuker. El timer puede iniciarse, frenarse o resetarse. Se cuenta también con comando para reiniciar todo el tablero.
No está implementado interfaz Wifi con Access Point y tampoco la chicharra o campana sonora que acompaña a eventos especificos del timer.

# TO-DO
1. La comparacion del comando a ejecutar cuando se recibe una solicitud HTTP no debería estar hardcodeada.
2. Emprolijar el llamado a funcion refreshScoreboard(). Parece realizar muchas tareas.
3. En conjunto con 2), rediseñar setBufferTx(). Esta orientada solo a enviar dataFrame al tablero.
4. Renombrar dataFrame para que represente que son datos para el tablero. (ahora apareceran datos para la app movil tambien, con datos del tablero, aunque se enviaran por wifi).
5. Definir metodo de envio de datos de tablero a front-end -> ¿Acciones por solicitudes HTTP y actualización de timer por WebSocket? Definir con el desarrollador del front-end.
6. 