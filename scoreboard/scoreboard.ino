#include "WiFi.h"
#include "ESPAsyncWebServer.h"

/***************************************************************************/
/******************************** DEFINES **********************************/

#define DATA_FRAME_ROWS 20      // Filas de la matriz dataFrame a enviar a placa controladora. Filas -> header, comando, dato, ...
#define DATA_FRAME_COLUMNS 2    // Columnas de la matriz dataFrame a enviar a placa controladora (2 columnas -> pares [key, value])
#define VALUE 1                 // Columna en la que se encuentra el valor del par [key, value]
#define DECREASE 0
#define INCREASE 1
#define TEAM_1 0
#define TEAM_2 1
#define SECOND_IN_MICROS 1000000
#define DOT_VALUE 0x80
#define RX_MAX_LONG 50
#define TX_MAX_LONG 50
#define MAX_CONNECTIONS 1

/***************************************************************************/
/******************************** GLOBAL ***********************************/

bool timerUpdated = false;
bool cmdReceived = false;
const char* cmdIncreaseScoreT1 = "+t1";
const char* cmdIncreaseScoreT2 = "+t2";
const char* cmdDecreaseScoreT1 = "-t1";
const char* cmdDecreaseScoreT2 = "-t1";
const char* cmdIncreaseChuker = "+c";
const char* cmdDecreaseChuker = "-c";
const char* cmdStopTimer = "stop_timer";
const char* cmdStartTimer = "start_timer";
const char* cmdResetTimer = "reset_timer";
const char* cmdResetAll = "reset_all";

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
  PROCESS_COMMAND,
  EXECUTE_COMMAND,
  INIT,
  ERROR
} main_state = INIT;

enum timer_state_t{
  RUNNING,
  STOPPED,
  FINISHED
};

enum command_t
{
  INC_SCORE_T1,
  INC_SCORE_T2,
  DEC_SCORE_T1,
  DEC_SCORE_T2,
  INC_CHUKER,
  DEC_CHUKER,
  START_TIMER,
  STOP_TIMER,
  RESET_TIMER,
  RESET_ALL,
  NOTHING_TO_DO
} command = NOTHING_TO_DO;

typedef enum
{
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
  SCORE_TEAM2_UNIDAD,
  SCORE_TEAM2_DECENA,
  SCORE_TEAM1_UNIDAD,
  SCORE_TEAM1_DECENA,
  CHUKER,
  DATA_END,
  CHECKSUM,
  FRAME_END
} data_frame_keys;

/***************************************************************************/
/****************************** IRQ_HANDLERS *******************************/
void IRAM_ATTR Timer0_ISR()
{
  timerUpdated = true;
}

/***************************************************************************/
/********************************** SETUP **********************************/

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
  delay(100);
  
  /* WIFI ACCESS POINT */
  Serial.println("Setting AP (Access Point) ...");
  if(!WiFi.softAP(ssid, password, 1, false, MAX_CONNECTIONS)){ Serial.println("Something went wrong!"); }

  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);
  
  server.on("/timer", HTTP_GET, [](AsyncWebServerRequest* request){
    const int paramQty = request->params();
    if(paramQty < 1){
      Serial.println("Faltan parametros.");
      request->send(400);
      return;
    }
    AsyncWebParameter* p_0 = request->getParam(0);
    const String cmd = p_0->value();
    if(cmd == "stop"){
      Serial.println("Solicitud de frenar el timer");
      command = STOP_TIMER;
    }
    else if(cmd == "start"){
      Serial.println("Solicitud de iniciar el timer");
      command = START_TIMER;
    }
    else if(cmd == "reset"){
      Serial.println("Solicitud de resetear el timer");
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
      Serial.println("Error en parametro->cmd");
      request->send(404);
      return;
    }
    cmdReceived = true;
    request->send(200);
  });

  server.on("/score", HTTP_GET, [](AsyncWebServerRequest* request){
    const int paramQty = request->params();
    if(paramQty < 2){
      Serial.println("Faltan parametros.");
      request->send(400);
      return;
    }
    AsyncWebParameter* p_0 = request->getParam(0);
    AsyncWebParameter* p_1 = request->getParam(1);
    const String cmd = p_0->value();
    const String param = p_1->value();
    if(cmd == "up"){
      if(param == "t1"){
        Serial.println("Solicitud de incrementar puntaje team_1.");
        command = INC_SCORE_T1;
      }
      else if(param == "t2"){
        Serial.println("Solicitud de incrementar puntaje team_2.");
        command = INC_SCORE_T2;
      }
      else{
        Serial.println("Error en parametro->param");
        request->send(404);
      }
    }
    else if(cmd == "down"){
      if(param == "t1"){
        Serial.println("Solicitud de decrementar puntaje team_1.");
        command = DEC_SCORE_T1;
      }
      else if(param == "t2"){
        Serial.println("Solicitud de decrementar puntaje team_2.");
        command = DEC_SCORE_T2;
      }
      else{
        Serial.println("Error en parametro->param");
        request->send(404);
      }
    }
    else{
      Serial.println("Error en parametro->cmd");
      request->send(404);
    }
    cmdReceived = true;
    request->send(200);
  });

  server.on("/chuker", HTTP_GET, [](AsyncWebServerRequest* request){
    const int paramQty = request->params();
    if(paramQty < 1){
      Serial.println("Faltan parametros.");
      request->send(400);
      return;
    }
    AsyncWebParameter* p_0 = request->getParam(0);
    const String cmd = p_0->value();
    Serial.println("Param_0 value: " + cmd);
    if(cmd == "up"){
      Serial.println("Solicitud de incrementar el chuker.");
      command = INC_CHUKER;
    }
    else if(cmd == "down"){
      Serial.println("Solicitud de decrementar el chuker.");
      command = DEC_CHUKER;
    }
    else{
      Serial.println("Error en parametro->cmd");
      request->send(404);
    }
    cmdReceived = true;
    request->send(200);
  });
  server.begin();

  delay(100);
}

/***************************************************************************/
/******************************** FUNCTIONS ********************************/

char genChecksum(char dataFrame[DATA_FRAME_ROWS][DATA_FRAME_COLUMNS]) {
  int checksum = 0, i;
  // Para el checksum, se suman los bytes de dataFrame desde el byte COMMAND hastas DATA_END inclusive
  for(i=COMMAND; i <= DATA_END; i++){
    checksum += dataFrame[i][VALUE];  // Los datos se encuentran en la 2da columna -> pares [key, value]
  }
  return (char)(checksum & 0xFF); // el checksum es byte menos significativo
}

void setDataFrame(scoreboard_t* scoreboard, char dataFrame[DATA_FRAME_ROWS][DATA_FRAME_COLUMNS]){
  // Se convierten valores numéricos a valores ASCII y se asignan al dataFrame a enviar al tablero
  dataFrame[TIMER_MM_DECENA][VALUE] = (char)(scoreboard->timer.value.mm / 10 + '0');
  dataFrame[TIMER_MM_UNIDAD][VALUE] = (char)(scoreboard->timer.value.mm % 10 + '0') + DOT_VALUE;
  dataFrame[TIMER_SS_DECENA][VALUE] = (char)(scoreboard->timer.value.ss / 10 + '0') + DOT_VALUE;
  dataFrame[TIMER_SS_UNIDAD][VALUE] = (char)(scoreboard->timer.value.ss % 10 + '0');
  dataFrame[SCORE_TEAM2_UNIDAD][VALUE] = (char)(scoreboard->score[TEAM_2] % 10 + '0');
  dataFrame[SCORE_TEAM2_DECENA][VALUE] = (char)(scoreboard->score[TEAM_2] / 10 + '0');
  dataFrame[SCORE_TEAM1_UNIDAD][VALUE] = (char)(scoreboard->score[TEAM_1] % 10 + '0');
  dataFrame[SCORE_TEAM1_DECENA][VALUE] = (char)(scoreboard->score[TEAM_1] / 10 + '0');
  dataFrame[CHUKER][VALUE] = (char)(scoreboard->chuker % 10 + '0');
  dataFrame[CHECKSUM][VALUE] = genChecksum(dataFrame);
}

unsigned int setBufferTx(char* bufferTx, char dataFrame[DATA_FRAME_ROWS][DATA_FRAME_COLUMNS]) {
  char i = 0;
  unsigned int bytes_to_transfer = 0;
  for(i=0; i < DATA_FRAME_ROWS; i++){
    bufferTx[i] = dataFrame[i][VALUE];
    bytes_to_transfer++;
  }
  return bytes_to_transfer;
}

// main_state_t pool_serial_rx(char* bufferRx){
//   static byte index = 0;
//   char endMarker = '\n';
//   char newChar;
//   if (Serial.available() > 0) {
//     newChar = Serial.read();
//     if(newChar != endMarker){
//       bufferRx[index] = newChar;
//       index++;
//       if(index >= RX_MAX_LONG){
//         index = RX_MAX_LONG - 1;
//       }
//     }
//     else{
//       bufferRx[index] = '\0';
//       index = 0;
//       return PROCESS_COMMAND;
//     }
//   }  
//   return IDLE;
// }

command_t processCommand(char* bufferRx){
  char* command = bufferRx;
  strcpy(command, bufferRx);
  Serial.print("Received: ");
  Serial.println(command);
  if(strcmp(command, cmdIncreaseScoreT1) == 0){ return INC_SCORE_T1; }
  else if(strcmp(command, cmdIncreaseScoreT2) == 0){ return INC_SCORE_T2; }
  else if(strcmp(command, cmdDecreaseScoreT1) == 0){ return DEC_SCORE_T1; }
  else if(strcmp(command, cmdDecreaseScoreT2) == 0){ return DEC_SCORE_T2; }
  else if(strcmp(command, cmdIncreaseChuker) == 0){ return INC_CHUKER; }
  else if(strcmp(command, cmdDecreaseChuker) == 0){ return DEC_CHUKER; }
  else if(strcmp(command, cmdStartTimer) == 0){ return START_TIMER; }
  else if(strcmp(command, cmdStopTimer) == 0){ return STOP_TIMER; }
  else if(strcmp(command, cmdResetTimer) == 0){ return RESET_TIMER; }
  else if(strcmp(command, cmdResetAll) == 0){ return RESET_ALL; }
  else{ return NOTHING_TO_DO; }
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

void setDefaultValues(scoreboard_t* scoreboard, timer_state_t* timerState){
  scoreboard->score[TEAM_1] = 0;
  scoreboard->score[TEAM_2] = 0;
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

void refreshScoreboard(scoreboard_t* scoreboard, char dataFrame[DATA_FRAME_ROWS][DATA_FRAME_COLUMNS], char* bufferTx){
  unsigned int DATA_FRAME_BYTES = 0;
  setDataFrame(scoreboard, dataFrame);                        // setear la trama de datos a enviar
  DATA_FRAME_BYTES = setBufferTx((char*)bufferTx, dataFrame); // setear buffer para enviar por serial
  Serial.write(bufferTx, DATA_FRAME_BYTES);                   // enviar data
}

/************************************************************ INFINITE LOOP ************************************************************/
void loop() {
  char bufferRx[RX_MAX_LONG];
  char bufferTx[TX_MAX_LONG];
  timer_state_t timerState = STOPPED;
  scoreboard_t scoreboard;
  char dataFrame[DATA_FRAME_ROWS][DATA_FRAME_COLUMNS] = {
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
    { SCORE_TEAM2_UNIDAD, 0x30 },
    { SCORE_TEAM2_DECENA, 0x30 },
    { SCORE_TEAM1_UNIDAD, 0x30 },
    { SCORE_TEAM1_DECENA, 0x30 },
    { CHUKER, 0x30 },
    { DATA_END, 0xFF },
    { CHECKSUM, 0x9D },
    { FRAME_END, 0x7F },
  };

  /****************************** STATE MACHINE LOOP *******************************/

  while(1){
    switch(main_state){
      case IDLE:
        // A la espera de comando por UART
        // main_state = pool_serial_rx(bufferRx);
        if(timerUpdated){
          timerState = updateTimer(&scoreboard);
          refreshScoreboard(&scoreboard, dataFrame, bufferTx);
          main_state = IDLE;
          timerUpdated = false;
        }
        else if(cmdReceived){
          main_state = PROCESS_COMMAND;
        }
        break;
      case PROCESS_COMMAND:
        // command = processCommand(bufferRx);
        if(command != NOTHING_TO_DO){ main_state = EXECUTE_COMMAND; }
        else{ main_state = ERROR; }
        break;
      case EXECUTE_COMMAND:
        switch(command){
          case INC_SCORE_T1:
            updateScores(INCREASE, TEAM_1, &scoreboard);
            refreshScoreboard(&scoreboard, dataFrame, bufferTx);
            main_state = IDLE;
            break;
          case INC_SCORE_T2:
            updateScores(INCREASE, TEAM_2, &scoreboard);
            refreshScoreboard(&scoreboard, dataFrame, bufferTx);
            main_state = IDLE;
            break;
          case DEC_SCORE_T1:
            updateScores(DECREASE, TEAM_1, &scoreboard);
            refreshScoreboard(&scoreboard, dataFrame, bufferTx);
            main_state = IDLE;
            break;
          case DEC_SCORE_T2:
            updateScores(DECREASE, TEAM_2, &scoreboard);
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
            setDefaultValues(&scoreboard, &timerState);
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