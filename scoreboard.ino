#include "WiFi.h"
#include "ESPAsyncWebServer.h"
#include "SPIFFS.h"
#include "driver/dac.h"

/***************************************************************************/
/******************************** DEFINES **********************************/
#define MAX_CONNECTIONS 1
#define LOG_TO_CONSOLE 0
#define ALARM_FREQ 261 // Aprox. un DO
#define ALARM_TIMEOUT 2

/***************************************************************************/
/******************************** GLOBAL ***********************************/
const byte DATA_FRAME_ROWS = 20;   // Filas de la matriz dataFrame a enviar a placa controladora. Filas -> header, comando, dato, ...
const byte DAC_SAMPLE_VALUES = 100;
const int TICKS_FOR_DAC = 10000000 / (ALARM_FREQ * DAC_SAMPLE_VALUES);
const byte TX_MAX_LONG = 50; 
const byte DOT_VALUE = 0x80;
const int SECOND_IN_MICROS = 1000000;
const byte DECREASE = 0;
const byte INCREASE = 1;
const byte LOCAL = 0;
const byte VISITOR = 1;
bool timerValueUpdate = false;
bool cmdReceived = false;
const char *ssid = "Polo Points";
const char *password = "12345678";
AsyncWebServer server(80);

// LookUpTable Senoidal & Index
byte SampleIdx = 0;
const byte sineLookupTable[DAC_SAMPLE_VALUES] = {
0x80, 0x88, 0x8f, 0x97, 0x9f, 0xa7, 0xae, 0xb6, 0xbd, 0xc4, 0xca, 0xd1, 0xd7, 0xdc, 0xe2, 0xe7, 0xeb, 0xef, 0xf3, 0xf6,
0xf9, 0xfb, 0xfd, 0xfe, 0xff, 0xff, 0xff, 0xfe, 0xfd, 0xfb, 0xf9, 0xf6, 0xf3, 0xef, 0xeb, 0xe7, 0xe2, 0xdc, 0xd7, 0xd1,
0xca, 0xc4, 0xbd, 0xb6, 0xae, 0xa7, 0x9f, 0x97, 0x8f, 0x88, 0x80, 0x77, 0x70, 0x68, 0x60, 0x58, 0x51, 0x49, 0x42, 0x3b,
0x35, 0x2e, 0x28, 0x23, 0x1d, 0x18, 0x14, 0x10, 0x0c, 0x09, 0x06, 0x04, 0x02, 0x01, 0x00, 0x00, 0x00, 0x01, 0x02, 0x04,
0x06, 0x09, 0x0c, 0x10, 0x14, 0x18, 0x1d, 0x23, 0x28, 0x2e, 0x35, 0x3b, 0x42, 0x49, 0x51, 0x58, 0x60, 0x68, 0x70, 0x77};

/***************************************************************************/
/********************************* TIMER ***********************************/
hw_timer_t *Timer0_cfg = NULL;
hw_timer_t *Timer1_cfg = NULL;

/***************************************************************************/
/****************************** DATA TYPES *********************************/
struct alarm_t{
  bool active = false;
  bool enabled = true;
  byte seconds = ALARM_TIMEOUT;
} alarm_obj;

struct _timer_t{
  int mm;
  int ss;
};

struct scoreboard_t{
  int score[2] = {0, 0};
  int chukker = 1;
  struct __timer_t
  {
    _timer_t value = { 6,30 };
    _timer_t initValue = { 6,30 };  // se usa para el comando reset_timer
    _timer_t halftime = { 3,0 };    // 3' 0'' de descanso en intervalos
  } timer;
} scoreboard;

enum main_state_t{
  IDLE,
  REFRESH_SCOREBOARD,
  INIT
} main_state = INIT;

enum timer_state_t{
  STOPPED,
  RUNNING,
  FINISHED
} timer_state = STOPPED;

enum game_state_t{
  IN_PROGRESS,
  HALFTIME
} game_state = IN_PROGRESS;

enum command_t{
  INC_SCORE_LOCAL     = 1,
  INC_SCORE_VISITOR   = 2,  
  DEC_SCORE_LOCAL     = 3,
  DEC_SCORE_VISITOR   = 4,
  INC_CHUKKER         = 5,
  DEC_CHUKKER         = 6,
  START_TIMER         = 7,
  STOP_TIMER          = 8,
  RESET_TIMER         = 9,
  SET_CURRENT_TIMER   = 10,
  SET_DEFAULT_TIMER   = 11,
  SET_HALFTIME_TIMER  = 12,
  RESET_ALL           = 13
};

enum request_status_t{
  STATUS_OK = 200,
  STATUS_ACCEPTED = 202,
  STATUS_BAD_REQUEST = 400,
  STATUS_NOT_FOUND = 404
};

enum data_frame_index_t{
  HEADER,
  COMMAND,
  ADDRESS,
  RESPONSE,
  RESERVED_1,
  FLASH,
  RESERVED_2,
  RESERVED_3,
  SCORE_LOCAL_DECENA,
  SCORE_LOCAL_UNIDAD,
  TIMER_MM_DECENA,
  TIMER_MM_UNIDAD,
  CHUKKER,
  TIMER_SS_DECENA,
  TIMER_SS_UNIDAD,
  SCORE_VISITOR_DECENA,
  SCORE_VISITOR_UNIDAD,
  DATA_END,
  CHECKSUM,
  FRAME_END
};

/***************************************************************************/
/***************************** DECLARATIONS ********************************/
byte genChecksum(byte *dataFrame);
void setDataFrame(scoreboard_t *scoreboard, byte *dataFrame);
unsigned int setBufferTx(byte *bufferTx, byte *dataFrame);
void startAlarm();
void stopAlarm();
timer_state_t refreshTimer(scoreboard_t *scoreboard);
void startTimer();
void stopTimer();
bool setTimerValue(int mm, int ss, int action);
void resetTimer();
void updateScores(int action, int team);
void updateChukker(int action);
void resetScoreboard();
void setDataFrameHeaders(byte *dataFrame);
void refreshScoreboard(scoreboard_t *scoreboard, byte *dataFrame, byte *bufferTx);
String getScoreboard_toString();

/***************************************************************************/
/****************************** IRQ_HANDLERS *******************************/
void IRAM_ATTR Timer0_ISR()
{
  timerValueUpdate = true;
  if(alarm_obj.active == true){
    alarm_obj.seconds--;
  }
}

void IRAM_ATTR Timer1_ISR()
{
  // Setear valor del DAC segun tabla de valores de señal senoidal
  dac_output_voltage(DAC_CHANNEL_1, sineLookupTable[SampleIdx++]);
  if(SampleIdx >= DAC_SAMPLE_VALUES) SampleIdx = 0;
}

/**************************************************************** SETUP ****************************************************************/
void setup()
{
  // put your setup code here, to run once:
  Serial.begin(9600);

  // Timer 0: timer del tablero
  Timer0_cfg = timerBegin(0, 80, true);
  timerAttachInterrupt(Timer0_cfg, &Timer0_ISR, true);
  timerAlarmWrite(Timer0_cfg, SECOND_IN_MICROS, true);
  timerAlarmEnable(Timer0_cfg);
  // El timer inicia automaticamente con timerBegin, por lo tanto se frena y reinicia contador
  timerStop(Timer0_cfg);
  timerWrite(Timer0_cfg, 0);

  // Timer 1: usado para el DAC (salida de audio)
  Timer1_cfg = timerBegin(1, 8, true);
  timerAttachInterrupt(Timer1_cfg, &Timer1_ISR, true);
  timerAlarmWrite(Timer1_cfg, TICKS_FOR_DAC, true);    // timer_interrup 1MHz -> DAC 100KSPS (con arreglo de 100 muestras)
  timerAlarmEnable(Timer1_cfg);
  // Se inhabilitan componentes, solo se habilitan en determinado momento
  timerStop(Timer1_cfg);
  dac_output_disable(DAC_CHANNEL_1);

  // Inicializar SPIFFS
  if (!SPIFFS.begin(true)){
    if(LOG_TO_CONSOLE){ Serial.println("Something went wrong mountint SPIFFS."); }
    return;
  }

  /* WIFI ACCESS POINT */
  if(LOG_TO_CONSOLE){ Serial.println("Setting AP (Access Point) ..."); }
  if (!WiFi.softAP(ssid, password, 1, false, MAX_CONNECTIONS)){
    if(LOG_TO_CONSOLE){ Serial.println("Something went wrong configuring AP!"); }
  }

  IPAddress IP = WiFi.softAPIP();
  if(LOG_TO_CONSOLE){
    Serial.print("AP IP address: ");
    Serial.println(IP);
  }
  
  /***************************************************************************/
  /***************************** HTTP REQUEST ********************************/
  // Ruta a index.html
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html", String(), false);
  });

  // Ruta a style.css
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/style.css", "text/css");
  });

  // Ruta a index.js
  server.on("/index.js", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.js", "text/html");
  });

  // Timer
  server.on("/timer/set", HTTP_GET, [](AsyncWebServerRequest * request){
    const int paramQty = request->params();
    String data;
    if(LOG_TO_CONSOLE){ Serial.println("Request to set timer value"); }
    if(paramQty < 3){
      if(LOG_TO_CONSOLE){ Serial.println("Not enought parameters."); }
      request->send(STATUS_BAD_REQUEST);
      return;
    }
    if(!(request->hasParam("mm") && request->hasParam("ss") && request->hasParam("cmd"))){
      request->send(STATUS_BAD_REQUEST);
      return;
    }
    AsyncWebParameter* p_0 = request->getParam("mm");  
    AsyncWebParameter* p_1 = request->getParam("ss");
    AsyncWebParameter* p_2 = request->getParam("cmd");
    int mm = (p_0->value()).toInt();
    int ss = (p_1->value()).toInt();
    int cmd = (p_2->value()).toInt();
    if(setTimerValue(mm, ss, cmd)){
      data = getScoreboard_toString();
      request->send(STATUS_ACCEPTED, "text/plain", data);
      cmdReceived = true;
    }
    else{ request->send(STATUS_BAD_REQUEST); }
  });
  
  server.on("/timer", HTTP_GET, [](AsyncWebServerRequest *request){
    const int paramQty = request->params();
    String data;
    if(paramQty < 1){
      if(LOG_TO_CONSOLE){ Serial.println("Not enought parameters."); }
      request->send(STATUS_BAD_REQUEST);
      return;
    }
    if(!(request->hasParam("cmd"))){
      request->send(STATUS_BAD_REQUEST);
      return;
    }
    AsyncWebParameter* p_0 = request->getParam("cmd");
    int cmd = (p_0->value()).toInt();
    if(cmd == START_TIMER){
      if(LOG_TO_CONSOLE){ Serial.println("Request to start the timer."); }
      startTimer();
      cmdReceived = true;
      request->send(STATUS_ACCEPTED);
    }
    else if(cmd == STOP_TIMER){
      if(LOG_TO_CONSOLE){ Serial.println("Request to stop the timer."); }
      stopTimer();
      cmdReceived = true;
      request->send(STATUS_ACCEPTED);
    }
    else if(cmd == RESET_TIMER){
      if(LOG_TO_CONSOLE){ Serial.println("Request to reset the timer."); }
      cmdReceived = true;
      resetTimer();
      data = getScoreboard_toString();
      request->send(STATUS_ACCEPTED, "text/plain", data);
    }
    else{
      if(LOG_TO_CONSOLE){ Serial.println("Parameter error -> cmd"); }
      request->send(STATUS_BAD_REQUEST);
    }
  });
  
  // Puntajes
  server.on("/score", HTTP_GET, [](AsyncWebServerRequest *request){
    const int paramQty = request->params();
    String data;
    if(paramQty < 1){
      if(LOG_TO_CONSOLE){ Serial.println("Not enought parameters."); }
      request->send(STATUS_BAD_REQUEST);
      return;
    }
    if(!(request->hasParam("cmd"))){
      request->send(STATUS_BAD_REQUEST);
      return;
    }
    AsyncWebParameter* p_0 = request->getParam("cmd");
    int cmd = (p_0->value()).toInt();
    if(cmd == INC_SCORE_VISITOR){
        if(LOG_TO_CONSOLE){ Serial.println("Request to increase visitor score."); }
        updateScores(INCREASE, VISITOR);
    }
    else if(cmd == INC_SCORE_LOCAL){
      if(LOG_TO_CONSOLE){ Serial.println("Request to increase local score."); }
      updateScores(INCREASE, LOCAL);
    }
    else if(cmd == DEC_SCORE_VISITOR){
      if(LOG_TO_CONSOLE){ Serial.println("Request to decrease visitor score."); }
      updateScores(DECREASE, VISITOR);
    }
    else if(cmd == DEC_SCORE_LOCAL){
      if(LOG_TO_CONSOLE){ Serial.println("Request to decrease local score."); }
      updateScores(DECREASE, LOCAL);
    }
    else{
      if(LOG_TO_CONSOLE){ Serial.println("Parameter error -> cmd"); }
      request->send(STATUS_BAD_REQUEST);
      return;
    }
    cmdReceived = true;
    data = getScoreboard_toString();
    request->send(STATUS_ACCEPTED, "text/plain", data);
  });

  // Chukker
  server.on("/chukker", HTTP_GET, [](AsyncWebServerRequest *request){
    const int paramQty = request->params();
    String data;
    if(paramQty < 1){
      if(LOG_TO_CONSOLE){ Serial.println("Not enought parameters."); }
      request->send(STATUS_BAD_REQUEST);
      return;
    }
    if(!(request->hasParam("cmd"))){
      request->send(STATUS_BAD_REQUEST);
      return;
    }
    AsyncWebParameter* p_0 = request->getParam("cmd");
    int cmd = (p_0->value()).toInt();
    if(cmd == INC_CHUKKER){
      if(LOG_TO_CONSOLE){ Serial.println("Request to increase chukker."); }
      updateChukker(INCREASE);
    }
    else if(cmd == DEC_CHUKKER){
      if(LOG_TO_CONSOLE){ Serial.println("Request to decrease chukker."); }
      updateChukker(DECREASE);
    }
    else{
      if(LOG_TO_CONSOLE){ Serial.println("Parameter error -> cmd"); }
      request->send(STATUS_BAD_REQUEST);
      return;
    }
    cmdReceived = true;
    data = getScoreboard_toString();
    request->send(STATUS_ACCEPTED, "text/plain", data);
  });

  // Reset tablero a valores default
  server.on("/reset", HTTP_GET, [](AsyncWebServerRequest *request){
    String data;
    if(LOG_TO_CONSOLE){ Serial.println("Request to reset all scoreboard values."); }
    resetScoreboard();
    cmdReceived = true;
    data = getScoreboard_toString();
    request->send(STATUS_ACCEPTED, "text/plain", data);
  });

  // Obtener datos de tablero como string
  server.on("/scoreboard", HTTP_GET, [](AsyncWebServerRequest *request){
    if(LOG_TO_CONSOLE){ Serial.println("Sending board data ..."); }
    String data = getScoreboard_toString();
    request->send(STATUS_OK, "text/plain", data);
  });

  // Rutas no definidas
  server.onNotFound([](AsyncWebServerRequest *request){
    if(LOG_TO_CONSOLE){ Serial.println("Request for undefined route."); }
    request->send(STATUS_NOT_FOUND, "text/plain", "Page not found.");
  });

  server.begin();
  delay(100);
}

/************************************************************ INFINITE LOOP ************************************************************/
void loop()
{
  byte bufferTx[TX_MAX_LONG];
  byte dataFrame[DATA_FRAME_ROWS];
  //bool init = false;
  byte i;
  
  /*********************************************************************************/
  /****************************** STATE MACHINE LOOP *******************************/
  while (1)
  {
    switch (main_state)
    {
    case IDLE:
      // Pasado un segundo, con el timer activo, se actualiza el tablero
      if (timerValueUpdate){
        timerValueUpdate = false;
        timer_state = refreshTimer();
        main_state = REFRESH_SCOREBOARD;
      }
      // A la espera de comando por HTTP Request
      if (cmdReceived){
        cmdReceived = false;
        main_state = REFRESH_SCOREBOARD;
      }
      break;
    case REFRESH_SCOREBOARD:
      refreshScoreboard(&scoreboard, dataFrame, bufferTx);
      main_state = IDLE;
      break;
    case INIT:
      if(LOG_TO_CONSOLE){ Serial.println("Inicializando tablero ..."); }
      setDataFrameHeaders(dataFrame);
      for(i=0;i<5;i++){
        refreshScoreboard(&scoreboard, dataFrame, bufferTx);
        delay(1000);
      }
      //do{
      //  refreshScoreboard(&scoreboard, dataFrame, bufferTx); // nuevo intento de inicializar tablero
      //  if(Serial.available()){
      //    if(Serial.read() == 0xCC){ init = true; }
      //  }
      //  else{ delay(1000); }
      //}while(!init);
      //if(LOG_TO_CONSOLE){ Serial.println("Inicialización exitosa!"); }
      dataFrame[RESPONSE] = 0x01;   // comunicacion con placa controladora sin código
      main_state = IDLE;            // de retorno (solo necesario para inicializacion)                                    
      break;
    default:
      main_state = IDLE;
      break;
    }
    delay(10);
  }
}

/***************************************************************************/
/******************************** FUNCTIONS ********************************/
// Se inicializa el dataframe con los headers para comunicarse con la placa controladora
void setDataFrameHeaders(byte *dataFrame){
  dataFrame[HEADER] = 0x7F;
  dataFrame[COMMAND] = 0xDD;    // enviar data
  dataFrame[ADDRESS] = 0x00;    // broadcast
  dataFrame[RESPONSE] = 0x00;   // codigo de retorno habilitado (solo para inicializacion, confirmando comunicacion)
  dataFrame[RESERVED_1] = 0x00;
  dataFrame[FLASH] = 0x01;      // no guardar en flash
  dataFrame[RESERVED_2] = 0x00;
  dataFrame[RESERVED_3] = 0x00;
  dataFrame[DATA_END] = 0xFF;
  dataFrame[FRAME_END] = 0x7F;
}

// Genera el checksum para el data frame a enviar a la placa controladora de leds
byte genChecksum(byte *dataFrame)
{
  int checksum = 0;
  byte i;
  // Para el checksum, se suman los bytes de dataFrame desde el byte COMMAND hasta el DATA_END inclusive
  for (i = COMMAND; i <= DATA_END; i++)
  {
    checksum += dataFrame[i];
  }
  return (byte)(checksum & 0xFF); // el checksum es byte menos significativo
}

// Copia los datos de data frame a buffer a transmitir por serial
unsigned int setBufferTx(byte *bufferTx, byte *dataFrame)
{
  byte i = 0;
  unsigned int bytes_to_transfer = 0;
  for (i = 0; i < DATA_FRAME_ROWS; i++)
  {
    bufferTx[i] = dataFrame[i];
    bytes_to_transfer++;
  }
  return bytes_to_transfer;
}

// Iniciar alarma
void startAlarm(){
  if(alarm_obj.enabled == true){
    timerStart(Timer1_cfg);
    dac_output_enable(DAC_CHANNEL_1);
    alarm_obj.active = true;
    alarm_obj.enabled = false;
  }
}

// Detener alarma
void stopAlarm(){
  timerStop(Timer1_cfg);
  dac_output_disable(DAC_CHANNEL_1);
  alarm_obj.active = false;
  alarm_obj.seconds = ALARM_TIMEOUT;
}

// Actualiza timer, transcurrido un segundo
timer_state_t refreshTimer()
{
  // Apagar alarma si corresponde
  if(alarm_obj.active == true && alarm_obj.seconds <= 0) stopAlarm();

  // Actualizar timer
  scoreboard.timer.value.ss--;
  if (scoreboard.timer.value.mm == 0 && scoreboard.timer.value.ss == 0)
  {
    if(game_state == HALFTIME){
      // Detener y resetear el timer si finalizó el descanso
      timerStop(Timer0_cfg);
      timerWrite(Timer0_cfg, 0);
      return FINISHED;
    }
    else{
      game_state = HALFTIME;
      scoreboard.timer.value.mm = scoreboard.timer.halftime.mm;
      scoreboard.timer.value.ss = scoreboard.timer.halftime.ss;
    }
  }
  else if (scoreboard.timer.value.ss < 0)
  {
    // Si pasaron 60 segundos, decrementar minutos y resetear segundos.
    scoreboard.timer.value.ss = 59;
    if (scoreboard.timer.value.mm > 0)
    {
      scoreboard.timer.value.mm--;
    }
  }
  else if(game_state != HALFTIME && scoreboard.timer.value.mm == 0 && scoreboard.timer.value.ss == 10){
    // A los 10 segundos vuelve a sonar la alarma
    alarm_obj.enabled = true;
    startAlarm();
  }
  return RUNNING;
}

// Iniciar el timer
void startTimer()
{
  if (timer_state == STOPPED && !(scoreboard.timer.value.mm == 0 && scoreboard.timer.value.ss == 0))
  {
    // Iniciar timer
    timerStart(Timer0_cfg);
    timer_state = RUNNING;

    if(alarm_obj.enabled == true) startAlarm();
  }
}

// Frena el timer
void stopTimer()
{
  if (timer_state == RUNNING)
  {
    timerStop(Timer0_cfg);
    timer_state = STOPPED;

    if(alarm_obj.active == true) stopAlarm();
  }
}

// Resetear el timer a valor default y frenarlo
void resetTimer()
{
  scoreboard.timer.value.mm = scoreboard.timer.initValue.mm;
  scoreboard.timer.value.ss = scoreboard.timer.initValue.ss;

  // Resetear y frenar el timer
  timerStop(Timer0_cfg);
  timerWrite(Timer0_cfg, 0);
  timer_state = STOPPED;
  game_state = IN_PROGRESS;

  // Se detiene alarma y se habilita para futuro uso
  stopAlarm();
  alarm_obj.enabled = true;
}

// Setear valor en timer
bool setTimerValue(int mm, int ss, int action){
  if(timer_state == RUNNING || timer_state == FINISHED){ return false; }
  else if(mm == 0 && ss == 0){ return false; }
  else if(mm < 0 || mm > 59){ return false; }
  else if(ss < 0 || ss > 59){ return false; }
  if(action == SET_DEFAULT_TIMER){
    scoreboard.timer.initValue.mm = mm;
    scoreboard.timer.initValue.ss = ss;
    scoreboard.timer.value.mm = mm;
    scoreboard.timer.value.ss = ss;
  }
  else if (action == SET_CURRENT_TIMER){
    scoreboard.timer.value.mm = mm;
    scoreboard.timer.value.ss = ss;
  }
  else if (action == SET_HALFTIME_TIMER){
    scoreboard.timer.halftime.mm = mm;
    scoreboard.timer.halftime.ss = ss;
  }
  else{ return false; }
  return true;
}

// Actualizar puntajes, con accion y equipo que corresponda
void updateScores(int action, int team)
{
  if (action == INCREASE)
  {
    if (scoreboard.score[team] < 99){ scoreboard.score[team]++; }
    else{ scoreboard.score[team] = 0; }
  }
  else if (scoreboard.score[team] > 0){ scoreboard.score[team]--; }
}

// Actualizar chukker, segun la accion solicitada
void updateChukker(int action)
{
  if (action == INCREASE)
  {
    if (scoreboard.chukker < 9) { scoreboard.chukker++; }
    else{ scoreboard.chukker = 0; }
  }
  else if (scoreboard.chukker > 0){ scoreboard.chukker--; }
}

// Llevar tablero a valor default
void resetScoreboard()
{
  scoreboard.score[VISITOR] = 0;
  scoreboard.score[LOCAL] = 0;
  scoreboard.chukker = 1;
  scoreboard.timer.value.mm = scoreboard.timer.initValue.mm;
  scoreboard.timer.value.ss = scoreboard.timer.initValue.ss;

  // Resetear y frenar el timer
  timerStop(Timer0_cfg);
  timerWrite(Timer0_cfg, 0);
  timer_state = STOPPED;
  game_state = IN_PROGRESS;

  // Se detiene alarma y se habilita para futuro uso
  stopAlarm();
  alarm_obj.enabled = true;
}

// Actualizar datos en tablero, enviando los datos por serial a placa controladora
void refreshScoreboard(scoreboard_t *scoreboard, byte *dataFrame, byte *bufferTx)
{
  unsigned int DATA_FRAME_BYTES = 0;
  setDataFrame(scoreboard, dataFrame);                 // setear la trama de datos a enviar
  DATA_FRAME_BYTES = setBufferTx(bufferTx, dataFrame); // setear buffer para enviar por serial
  Serial.write(bufferTx, DATA_FRAME_BYTES);            // enviar data
}

// Se convierten valores numéricos a valores ASCII y se asignan al dataFrame a enviar al tablero
void setDataFrame(scoreboard_t *scoreboard, byte *dataFrame)
{
  dataFrame[TIMER_MM_DECENA] = (byte)(scoreboard->timer.value.mm / 10 + '0');
  dataFrame[TIMER_MM_UNIDAD] = (byte)(scoreboard->timer.value.mm % 10 + '0') + DOT_VALUE;
  dataFrame[TIMER_SS_DECENA] = (byte)(scoreboard->timer.value.ss / 10 + '0') + DOT_VALUE;
  dataFrame[TIMER_SS_UNIDAD] = (byte)(scoreboard->timer.value.ss % 10 + '0');
  dataFrame[SCORE_VISITOR_UNIDAD] = (byte)(scoreboard->score[VISITOR] % 10 + '0');
  dataFrame[SCORE_VISITOR_DECENA] = (byte)(scoreboard->score[VISITOR] / 10 + '0');
  dataFrame[SCORE_LOCAL_UNIDAD] = (byte)(scoreboard->score[LOCAL] % 10 + '0');
  dataFrame[SCORE_LOCAL_DECENA] = (byte)(scoreboard->score[LOCAL] / 10 + '0');
  dataFrame[CHUKKER] = (byte)(scoreboard->chukker % 10 + '0');
  dataFrame[CHECKSUM] = genChecksum(dataFrame);
}

// Transformar los datos del tablero en una cadena concatenada, para enviar a front-end
String getScoreboard_toString(){
  String s = String(scoreboard.score[0]);
  s += "," + String(scoreboard.score[1]);
  s += "," + String(scoreboard.chukker);
  s += "," + String(scoreboard.timer.value.mm);
  s += "," + String(scoreboard.timer.value.ss);
  s += "," + String(timer_state);
  s += "," + String(game_state);
  return s;
}
