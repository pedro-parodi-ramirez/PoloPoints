#include "WiFi.h"
#include "ESPAsyncWebServer.h"
#include "SPIFFS.h"

/***************************************************************************/
/******************************** DEFINES **********************************/
#define DECREASE 0
#define INCREASE 1
#define VISITOR 0
#define LOCAL 1
#define SECOND_IN_MICROS 1000000
#define DOT_VALUE 0x80
#define TX_MAX_LONG 50
#define MAX_CONNECTIONS 1

/***************************************************************************/
/******************************** GLOBAL ***********************************/
const byte DATA_FRAME_ROWS = 20;      // Filas de la matriz dataFrame a enviar a placa controladora. Filas -> header, comando, dato, ...
const byte DATA_FRAME_COLUMNS = 2;    // Columnas de la matriz dataFrame a enviar a placa controladora (2 columnas -> pares [key, value])
const int VALUE = 1;                  // Columna en la que se encuentra el valor del par [key, value]
bool timerUpdated = false;
bool cmdReceived = false;
const char* ssid = "ESP32-AccessPoint";
const char* password =  "12345678";
AsyncWebServer server(80);

/***************************************************************************/
/********************************* TIMER ***********************************/
hw_timer_t *Timer0_cfg = NULL;

/***************************************************************************/
/****************************** DATA TYPES *********************************/
struct _timer_t{
    int mm = 6;
    int ss = 30;
};

struct scoreboard_t{
  int score[2] = { 0, 0 };
  int chuker = 0;
  struct __timer_t{
    _timer_t value;
    _timer_t initValue; // se usa para el comando reset_timer
  } timer;
};

enum main_state_t{
  IDLE,
  EXECUTE_COMMAND,
  INIT,
  ERROR
} main_state = INIT;

enum timer_state_t{
  RUNNING,
  STOPPED,
  FINISHED
};

enum command_t{
  INC_SCORE_VISITOR,
  INC_SCORE_LOCAL,
  DEC_SCORE_VISITOR,
  DEC_SCORE_LOCAL,
  INC_CHUKER,
  DEC_CHUKER,
  START_TIMER,
  STOP_TIMER,
  RESET_TIMER,
  RESET_ALL,
  NOTHING_TO_DO
} command = NOTHING_TO_DO;

typedef enum{
  STATUS_OK = 200,
  STATUS_BAD_REQUEST = 404
};

typedef enum{
  HEADER,
  COMMAND,
  ADDRESS,
  RESPONSE,
  RESERVED_1,
  FLASH,
  RESERVED_2,
  RESERVED_3,
  TIMER_MM_DECENA,
  TIMER_MM_UNIDAD,
  TIMER_SS_DECENA,
  TIMER_SS_UNIDAD,
  SCORE_LOCAL_UNIDAD,
  SCORE_LOCAL_DECENA,
  SCORE_VISITOR_UNIDAD,
  SCORE_VISITOR_DECENA,
  CHUKER,
  DATA_END,
  CHECKSUM,
  FRAME_END
};

/***************************************************************************/
/***************************** DECLARATIONS ********************************/
byte genChecksum(byte dataFrame[DATA_FRAME_ROWS][DATA_FRAME_COLUMNS]);
void setDataFrame(scoreboard_t* scoreboard, byte dataFrame[DATA_FRAME_ROWS][DATA_FRAME_COLUMNS]);
unsigned int setBufferTx(byte* bufferTx, byte *dataFrame[DATA_FRAME_ROWS][DATA_FRAME_COLUMNS]);
timer_state_t updateTimer(scoreboard_t* scoreboard);
void startTimer(timer_state_t* timerState);
void stopTimer(timer_state_t* timerState);
void resetTimer(scoreboard_t* scoreboard, timer_state_t* timerState);
void resetScoreboard(scoreboard_t* scoreboard, timer_state_t* timerState);
void updateScores(int increaseScore, int team, scoreboard_t* scoreboard);
void updateChuker(int increaseScore, scoreboard_t* scoreboard);
void refreshScoreboard(scoreboard_t* scoreboard, byte dataFrame[DATA_FRAME_ROWS][DATA_FRAME_COLUMNS], byte* bufferTx);

/***************************************************************************/
/****************************** IRQ_HANDLERS *******************************/
void IRAM_ATTR Timer0_ISR()
{
  timerUpdated = true;
}

/**************************************************************** SETUP ****************************************************************/
void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  Timer0_cfg = timerBegin(0, 80, true);
  timerAttachInterrupt(Timer0_cfg, &Timer0_ISR, true);
  timerAlarmWrite(Timer0_cfg, SECOND_IN_MICROS, true);
  timerAlarmEnable(Timer0_cfg);
  // El timer inicia automaticamente con timerBegin, por lo tanto se frena y reinicia contador
  timerStop(Timer0_cfg);
  timerWrite(Timer0_cfg, 0);
  
  // Inicializar SPIFFS
  if(!SPIFFS.begin(true)){
    Serial.println("Something went wrong mountint SPIFFS.");
    return;
  }
  
  /* WIFI ACCESS POINT */
  Serial.println("Setting AP (Access Point) ...");
  if(!WiFi.softAP(ssid, password, 1, false, MAX_CONNECTIONS)){ Serial.println("Something went wrong configuring AP!"); }

  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);
  
  /***************************************************************************/
  /***************************** HTTP REQUEST ********************************/
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html", String(), false);
  });

  server.on("/timer", HTTP_GET, [](AsyncWebServerRequest* request){
    const int paramQty = request->params();
    if(paramQty < 1){
      Serial.println("Not enought parameters.");
      request->send(STATUS_BAD_REQUEST);
      return;
    }
    AsyncWebParameter* p_0 = request->getParam(0);
    int cmd = (p_0->value()).toInt();
    if(cmd == STOP_TIMER){
      Serial.println("Request to stop timer.");
      command = STOP_TIMER;
    }
    else if(cmd == START_TIMER){
      Serial.println("Request to start the timer.");
      command = START_TIMER;
    }
    else if(cmd == RESET_TIMER){
      Serial.println("Request to reset the timer.");
      command = RESET_TIMER;
    }
    // else if(cmd == "set"){
    //   if(paramQty < 2){
    //     request->send(400);
    //     return;
    //   }
    //   AsyncWebParameter* p_1 = request->getParam(1);
    //   const String param = p_1->value();
    //   Serial.println("Param_1 value: " + param);
    //   Serial.println("Solicitud de configurar tiempo del timer");
    // }
    else{
      Serial.println("Parameter error -> cmd");
      request->send(STATUS_BAD_REQUEST);
      return;
    }
    cmdReceived = true;
    request->send(STATUS_OK);
  });

  server.on("/score", HTTP_GET, [](AsyncWebServerRequest* request){
    const int paramQty = request->params();
    if(paramQty < 1){
      Serial.println("Not enought parameters.");
      request->send(STATUS_BAD_REQUEST);
      return;
    }
    AsyncWebParameter* p_0 = request->getParam(0);
    int cmd = (p_0->value()).toInt();
    if(cmd == INC_SCORE_VISITOR){
        Serial.println("Request to increase visitor score.");
        command = INC_SCORE_VISITOR;
    }
    else if(cmd == INC_SCORE_LOCAL){
      Serial.println("Request to increase local score.");
      command = INC_SCORE_LOCAL;
    }
    else if(cmd == DEC_SCORE_VISITOR){
      Serial.println("Request to decrease visitor score.");
      command = DEC_SCORE_VISITOR;
    }
    else if(cmd == DEC_SCORE_LOCAL){
      Serial.println("Request to decrease local score.");
      command = DEC_SCORE_LOCAL;
    }
    else{
      Serial.println("Parameter error -> param");
      request->send(STATUS_BAD_REQUEST);
      return;
    }
    cmdReceived = true;
    request->send(STATUS_OK);
  });

  server.on("/chuker", HTTP_GET, [](AsyncWebServerRequest* request){
    const int paramQty = request->params();
    if(paramQty < 1){
      Serial.println("Not enought parameters.");
      request->send(STATUS_BAD_REQUEST);
      return;
    }
    AsyncWebParameter* p_0 = request->getParam(0);
    int cmd = (p_0->value()).toInt();
    if(cmd == INC_CHUKER){
      Serial.println("Request to increase chuker.");
      command = INC_CHUKER;
    }
    else if(cmd == DEC_CHUKER){
      Serial.println("Request to decrease chuker.");
      command = DEC_CHUKER;
    }
    else{
      Serial.println("Parameter error -> cmd");
      request->send(STATUS_BAD_REQUEST);
      return;
    }
    cmdReceived = true;
    request->send(STATUS_OK);
  });

  server.on("/reset", HTTP_GET, [](AsyncWebServerRequest *request){
    command = RESET_ALL;
    cmdReceived = true;
    request->send(SPIFFS, "/index.html", String(), false);
  });
  server.begin();

  delay(100);
}

/************************************************************ INFINITE LOOP ************************************************************/
void loop() {
  byte bufferTx[TX_MAX_LONG];
  timer_state_t timerState = STOPPED;
  scoreboard_t scoreboard;
  byte dataFrame[DATA_FRAME_ROWS][DATA_FRAME_COLUMNS] = {
    { HEADER, 0x7F },
    { COMMAND, 0xDD },            // enviar data
    { ADDRESS, 0x00 },            // broadcast
    { RESPONSE, 0x01 },           // no return code
    { RESERVED_1, 0x00 },
    { FLASH, 0x01 },              // no guardar en flash
    { RESERVED_2, 0x00 },
    { RESERVED_3, 0x00 },
    { TIMER_MM_DECENA, 0x30 },
    { TIMER_MM_UNIDAD, 0x30 },
    { TIMER_SS_DECENA, 0x30 },
    { TIMER_SS_UNIDAD, 0x30 },
    { SCORE_LOCAL_UNIDAD, 0x30 },
    { SCORE_LOCAL_DECENA, 0x30 },
    { SCORE_VISITOR_UNIDAD, 0x30 },
    { SCORE_VISITOR_DECENA, 0x30 },
    { CHUKER, 0x30 },
    { DATA_END, 0xFF },
    { CHECKSUM, 0x9D },
    { FRAME_END, 0x7F },
  };

  /*********************************************************************************/
  /****************************** STATE MACHINE LOOP *******************************/
  while(1){
    switch(main_state){
      case IDLE:
        // Pasado un segundo, con el timer activo, se actualiza el tablero
        if(timerUpdated){
          timerState = updateTimer(&scoreboard);
          refreshScoreboard(&scoreboard, dataFrame, bufferTx);
          main_state = IDLE;
          timerUpdated = false;
        }
        // A la espera de comando por HTTP Request
        if(cmdReceived){
          main_state = EXECUTE_COMMAND;
        }
        break;
      case EXECUTE_COMMAND:
        switch(command){
          case INC_SCORE_VISITOR:
            updateScores(INCREASE, VISITOR, &scoreboard);
            refreshScoreboard(&scoreboard, dataFrame, bufferTx);
            main_state = IDLE;
            break;
          case INC_SCORE_LOCAL:
            updateScores(INCREASE, LOCAL, &scoreboard);
            refreshScoreboard(&scoreboard, dataFrame, bufferTx);
            main_state = IDLE;
            break;
          case DEC_SCORE_VISITOR:
            updateScores(DECREASE, VISITOR, &scoreboard);
            refreshScoreboard(&scoreboard, dataFrame, bufferTx);
            main_state = IDLE;
            break;
          case DEC_SCORE_LOCAL:
            updateScores(DECREASE, LOCAL, &scoreboard);
            refreshScoreboard(&scoreboard, dataFrame, bufferTx);
            main_state = IDLE;
            break;
          case INC_CHUKER:
            updateChuker(INCREASE, &scoreboard);
            refreshScoreboard(&scoreboard, dataFrame, bufferTx);
            main_state = IDLE;
            break;
          case DEC_CHUKER:
            updateChuker(DECREASE, &scoreboard);
            refreshScoreboard(&scoreboard, dataFrame, bufferTx);
            main_state = IDLE;
            break;
          case START_TIMER:
            startTimer(&timerState);
            main_state = IDLE;
            break;
          case STOP_TIMER:
            stopTimer(&timerState);
            main_state = IDLE;
            break;
          case RESET_TIMER:
            resetTimer(&scoreboard, &timerState);
            refreshScoreboard(&scoreboard, dataFrame, bufferTx);
            main_state = IDLE;
            break;
          case RESET_ALL:
            // Lleva todo el tablero a valores de inicio y frena el timer
            resetScoreboard(&scoreboard, &timerState);
            refreshScoreboard(&scoreboard, dataFrame, bufferTx);
            main_state = IDLE;
            break;
          default:
            command = NOTHING_TO_DO;
            break;
        }
        cmdReceived = false;
        command = NOTHING_TO_DO;
        break;
      case INIT:
        refreshScoreboard(&scoreboard, dataFrame, bufferTx);
        main_state = IDLE;
        break;
      case ERROR:
        Serial.println("Error: not known command");
        main_state = IDLE;
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
byte genChecksum(byte dataFrame[DATA_FRAME_ROWS][DATA_FRAME_COLUMNS]){
  int checksum = 0, i;
  // Para el checksum, se suman los bytes de dataFrame desde el byte COMMAND hasta el DATA_END inclusive
  for(i=COMMAND; i <= DATA_END; i++){
    checksum += dataFrame[i][VALUE];  // Los datos se encuentran en la 2da columna -> pares [key, value]
  }
  return (byte)(checksum & 0xFF); // el checksum es byte menos significativo
}

void setDataFrame(scoreboard_t* scoreboard, byte dataFrame[DATA_FRAME_ROWS][DATA_FRAME_COLUMNS]){
  // Se convierten valores numéricos a valores ASCII y se asignan al dataFrame a enviar al tablero
  dataFrame[TIMER_MM_DECENA][VALUE] = (byte)(scoreboard->timer.value.mm / 10 + '0');
  dataFrame[TIMER_MM_UNIDAD][VALUE] = (byte)(scoreboard->timer.value.mm % 10 + '0') + DOT_VALUE;
  dataFrame[TIMER_SS_DECENA][VALUE] = (byte)(scoreboard->timer.value.ss / 10 + '0') + DOT_VALUE;
  dataFrame[TIMER_SS_UNIDAD][VALUE] = (byte)(scoreboard->timer.value.ss % 10 + '0');
  dataFrame[SCORE_VISITOR_UNIDAD][VALUE] = (byte)(scoreboard->score[VISITOR] % 10 + '0');
  dataFrame[SCORE_VISITOR_DECENA][VALUE] = (byte)(scoreboard->score[VISITOR] / 10 + '0');
  dataFrame[SCORE_LOCAL_UNIDAD][VALUE] = (byte)(scoreboard->score[LOCAL] % 10 + '0');
  dataFrame[SCORE_LOCAL_DECENA][VALUE] = (byte)(scoreboard->score[LOCAL] / 10 + '0');
  dataFrame[CHUKER][VALUE] = (byte)(scoreboard->chuker % 10 + '0');
  dataFrame[CHECKSUM][VALUE] = genChecksum(dataFrame);
}

unsigned int setBufferTx(byte* bufferTx, byte dataFrame[DATA_FRAME_ROWS][DATA_FRAME_COLUMNS]){
  char i = 0;
  unsigned int bytes_to_transfer = 0;
  for(i=0; i < DATA_FRAME_ROWS; i++){
    bufferTx[i] = dataFrame[i][VALUE];
    bytes_to_transfer++;
  }
  return bytes_to_transfer;
}

timer_state_t updateTimer(scoreboard_t* scoreboard){
  scoreboard->timer.value.ss--;
  if(scoreboard->timer.value.mm == 0 && scoreboard->timer.value.ss == 0){
    // Detener y resetearlo el timer si llego a 00:00
    timerStop(Timer0_cfg);
    timerWrite(Timer0_cfg, 0);
    return FINISHED;
  }
  else if(scoreboard->timer.value.ss < 0){
    // Si pasaron 60 segundos, decrementar minutos y resetear segundos.
    scoreboard->timer.value.ss = 59;
    if(scoreboard->timer.value.mm > 0){ scoreboard->timer.value.mm--; }
  }
  return RUNNING;
}

void startTimer(timer_state_t* timerState){
  if(*timerState == STOPPED){
    timerStart(Timer0_cfg);
    *timerState = RUNNING;
  }
}

void stopTimer(timer_state_t* timerState){
  if(*timerState == RUNNING){
    timerStop(Timer0_cfg);
    *timerState = STOPPED;
  }
}

void resetTimer(scoreboard_t* scoreboard, timer_state_t* timerState){
  scoreboard->timer.value.mm = scoreboard->timer.initValue.mm;
  scoreboard->timer.value.ss = scoreboard->timer.initValue.ss;
  
  // Resetear y frenar el timer
  timerStop(Timer0_cfg);
  timerWrite(Timer0_cfg, 0);
  *timerState = STOPPED;
}

void resetScoreboard(scoreboard_t* scoreboard, timer_state_t* timerState){
  scoreboard->score[VISITOR] = 0;
  scoreboard->score[LOCAL] = 0;
  scoreboard->chuker = 0;
  scoreboard->timer.value.mm = 6;
  scoreboard->timer.value.ss = 50;
  
  // Resetear y frenar el timer
  timerStop(Timer0_cfg);
  timerWrite(Timer0_cfg, 0);
  *timerState = STOPPED;
}

void updateScores(int increaseScore, int team, scoreboard_t* scoreboard){
  if(increaseScore == INCREASE){
    if(scoreboard->score[team] < 99) { scoreboard->score[team]++; }
    else { scoreboard->score[team] = 0; }
  }
  else{
    if(scoreboard->score[team] > 0) { scoreboard->score[team]--; }
  }
}

void updateChuker(int increaseScore, scoreboard_t* scoreboard){
  if(increaseScore == INCREASE){
    if(scoreboard->chuker < 9) { scoreboard->chuker++; }
    else { scoreboard->chuker = 0; }
  }
  else{
    if(scoreboard->chuker > 0) { scoreboard->chuker--; }
  }
}

void refreshScoreboard(scoreboard_t* scoreboard, byte dataFrame[DATA_FRAME_ROWS][DATA_FRAME_COLUMNS], byte* bufferTx){
  unsigned int DATA_FRAME_BYTES = 0;
  setDataFrame(scoreboard, dataFrame);                     // setear la trama de datos a enviar
  DATA_FRAME_BYTES = setBufferTx(bufferTx, dataFrame);  // setear buffer para enviar por serial
  Serial.write(bufferTx, DATA_FRAME_BYTES);                                 // enviar data
}
