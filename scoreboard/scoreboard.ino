/***************************************************************************/
/******************************** DEFINES **********************************/

#define DATA_FRAME_ROWS 15      // Filas de la matriz dataFrame a enviar a placa controladora. Filas -> header, comando, dato, ...
#define DATA_FRAME_COLUMNS 2    // Columnas de la matriz dataFrame a enviar a placa controladora (2 columnas -> pares [key, value])
#define VALUE 1                 // Columna en la que se encuentra el valor del par [key, value]
#define DECREASE 0
#define INCREASE 1
#define TEAM_1 0
#define TEAM_2 1
#define SECOND_IN_MICROS 1000000
#define DOT_VALUE 0x80
#define RX_MAX_LONG 50

/***************************************************************************/
/********************************* TIMER ***********************************/

hw_timer_t *Timer0_cfg = NULL;

/***************************************************************************/
/****************************** DATA TYPES *********************************/

struct scoreboard_t{
  int score[2] = { 00, 00 };
  struct timer_t {
    int mm = 6;
    int ss = 50;
  } timer;
};

enum main_state_t
{
  WAITING_COMMAND,
  PROCESS_COMMAND,
  EXECUTE_COMMAND,
  UPDATE_TIMER,
  UPDATE_BOARD,
  ERROR
} main_state = UPDATE_BOARD;

enum timer_state_t{
  RUNNING,
  STOPPED
};

enum command_t
{
  INC_SCORE_T1,
  INC_SCORE_T2,
  DEC_SCORE_T1,
  DEC_SCORE_T2,
  INC_MATCH,
  DEC_MATCH,
  START_TIMER,
  STOP_TIMER,
  RESET_ALL,
  NOTHING_TO_DO
};

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
  // SCORE_TEAM1_DECENA,
  // SCORE_TEAM1_UNIDAD,
  // SCORE_TEAM2_DECENA,
  // SCORE_TEAM2_UNIDAD,
  TIMER_MM_DECENA,
  TIMER_MM_UNIDAD,
  TIMER_SS_DECENA,
  TIMER_SS_UNIDAD,
  DATA_END,
  CHECKSUM,
  FRAME_END
} data_frame_keys;

/***************************************************************************/
/****************************** IRQ_HANDLERS *******************************/
void IRAM_ATTR Timer0_ISR()
{
  main_state = UPDATE_TIMER;
}

/***************************************************************************/
/******************************** FUNCTIONS ********************************/

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
}

char genChecksum(char dataFrame[DATA_FRAME_ROWS][DATA_FRAME_COLUMNS]) {
  int checksum = 0, i;
  // Para el checksum, se suman los bytes de dataFrame desde el byte COMMAND hastas DATA_END inclusive
  for(i=COMMAND; i <= DATA_END; i++){
    checksum += dataFrame[i][VALUE];  // Los datos se encuentran en la 2da columna -> pares [key, value]
  }
  return (char)(checksum & 0xFF); // el checksum es byte menos significativo
}

void setDataFrame(scoreboard_t* scoreboard, char dataFrame[DATA_FRAME_ROWS][DATA_FRAME_COLUMNS]){
  // dataFrame[SCORE_TEAM1_DECENA][VALUE] = (char)(scoreboard->score[TEAM_1] / 10 + '0');
  // dataFrame[SCORE_TEAM1_UNIDAD][VALUE] = (char)(scoreboard->score[TEAM_1] % 10 + '0');
  // dataFrame[SCORE_TEAM2_DECENA][VALUE] = (char)(scoreboard->score[TEAM_2] / 10 + '0');
  // dataFrame[SCORE_TEAM2_UNIDAD][VALUE] = (char)(scoreboard->score[TEAM_2] % 10 + '0');
  dataFrame[TIMER_MM_DECENA][VALUE] = (char)(scoreboard->timer.mm / 10 + '0');
  dataFrame[TIMER_MM_UNIDAD][VALUE] = (char)(scoreboard->timer.mm % 10 + '0') + DOT_VALUE;
  dataFrame[TIMER_SS_DECENA][VALUE] = (char)(scoreboard->timer.ss / 10 + '0') + DOT_VALUE;
  dataFrame[TIMER_SS_UNIDAD][VALUE] = (char)(scoreboard->timer.ss % 10 + '0');
  dataFrame[CHECKSUM][VALUE] = genChecksum(dataFrame);
}

unsigned int setBufferTx(char* bufferTx, char dataFrame[DATA_FRAME_ROWS][DATA_FRAME_COLUMNS]) {
  char i = 0;
  unsigned int bytes_to_transfer = 0;
  for(i=0; i < DATA_FRAME_ROWS; i++){
    bufferTx[i] = dataFrame[i][VALUE];  // Los datos se encuentran en la 2da columna -> pares [key, value]
    bytes_to_transfer++;
  }
  return bytes_to_transfer;
}

main_state_t pool_serial_rx(char* bufferRx){
  static byte index = 0;
  char endMarker = '\n';
  char newChar;
  if (Serial.available() > 0) {
    newChar = Serial.read();
    if(newChar != endMarker){
      bufferRx[index] = newChar;
      index++;
      if(index >= RX_MAX_LONG){
        index = RX_MAX_LONG - 1;
      }
    }
    else{
      bufferRx[index] = '\0';
      index = 0;
      return PROCESS_COMMAND;
    }
  }  
  return WAITING_COMMAND;
}

command_t processCommand(char* bufferRx){
  char* command = bufferRx;
  strcpy(command, bufferRx);
  Serial.print("Received: ");
  Serial.println(command);
  if(strcmp(command, "INC_SCORE_T1") == 0){ return INC_SCORE_T1; }
  else if(strcmp(command, "INC_SCORE_T2") == 0){ return INC_SCORE_T2; }
  else if(strcmp(command, "DEC_SCORE_T1") == 0){ return DEC_SCORE_T1; }
  else if(strcmp(command, "DEC_SCORE_T2") == 0){ return DEC_SCORE_T2; }
  else if(strcmp(command, "INC_MATCH") == 0){ return INC_MATCH; }
  else if(strcmp(command, "DEC_MATCH") == 0){ return DEC_MATCH; }
  else if(strcmp(command, "START_TIMER") == 0){ return START_TIMER; }
  else if(strcmp(command, "STOP_TIMER") == 0){ return STOP_TIMER; }
  else if(strcmp(command, "RESET_ALL") == 0){ return RESET_ALL; }
  else{ return NOTHING_TO_DO; }
}

void updateTimer(scoreboard_t* scoreboard, timer_state_t* timerState){
  scoreboard->timer.ss--;
  if(scoreboard->timer.mm == 0 && scoreboard->timer.ss == 0){
    // Detener y resetearlo el timer si llego a 00:00
    timerStop(Timer0_cfg);
    timerWrite(Timer0_cfg, 0);
    *timerState = STOPPED;
  }
  else if(scoreboard->timer.ss < 0){
    // Si pasaron 60 segundos, decrementar minutos y resetear segundos.
    scoreboard->timer.ss = 59;
    if(scoreboard->timer.mm > 0){ scoreboard->timer.mm--; }
  }
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

void setDefaultValues(scoreboard_t* scoreboard, timer_state_t* timerState){
  scoreboard->score[TEAM_1] = 0;
  scoreboard->score[TEAM_2] = 0;
  scoreboard->timer.mm = 6;
  scoreboard->timer.ss = 50;
  
  // Resetear y frenar el timer
  timerStop(Timer0_cfg);
  timerWrite(Timer0_cfg, 0);
  *timerState = STOPPED;
}

void updateScore(int increaseScore, int team, scoreboard_t* scoreboard){
  if(increaseScore == INCREASE){
    if(scoreboard->score[team] < 99) { scoreboard->score[team]++; }
    else { scoreboard->score[team] = 0; }
  }
  else{
    if(scoreboard->score[team] > 0) { scoreboard->score[team]--; }
  }
}

/************************************************************ INFINITE LOOP ************************************************************/
void loop() {
  char bufferRx[RX_MAX_LONG];
  char bufferTx[50];
  scoreboard_t scoreboard;
  command_t command = NOTHING_TO_DO;
  timer_state_t timerState = STOPPED;

  char dataFrame[DATA_FRAME_ROWS][DATA_FRAME_COLUMNS] = {
    { HEADER, 0x7F },
    { COMMAND, 0xDD },            // enviar data
    { ADDRESS, 0x00 },            // broadcast
    { RESPONSE, 0x01 },           // no return code
    { RESERVED_1, 0x00 },
    { FLASH, 0x01 },              // no guardar en flash
    { RESERVED_2, 0x00 },
    { RESERVED_3, 0x00 },
    // { SCORE_TEAM1_DECENA, 0x30 },
    // { SCORE_TEAM1_UNIDAD, 0x30 },
    // { SCORE_TEAM2_DECENA, 0x30 },
    // { SCORE_TEAM2_UNIDAD, 0x30 },
    { TIMER_MM_DECENA, 0x30 },
    { TIMER_MM_UNIDAD, 0x30 },
    { TIMER_SS_DECENA, 0x30 },
    { TIMER_SS_UNIDAD, 0x30 },
    { DATA_END, 0xFF },
    { CHECKSUM, 0x9D },
    { FRAME_END, 0x7F },
  };
  unsigned int DATA_FRAME_BYTES = 0;

  /****************************** STATE MACHINE LOOP *******************************/

  while(1){
    switch(main_state){
      case WAITING_COMMAND:
        // A la espera de datos por UART
        main_state = pool_serial_rx(bufferRx);
        break;
      case PROCESS_COMMAND:
        command = processCommand(bufferRx);
        if(command != NOTHING_TO_DO){ main_state = EXECUTE_COMMAND; }
        else{ main_state = ERROR; }
        break;
      case EXECUTE_COMMAND:
        switch(command){
          case INC_SCORE_T1:
            updateScore(INCREASE, TEAM_1, &scoreboard);
            main_state = UPDATE_BOARD;
            break;
          case INC_SCORE_T2:
            updateScore(INCREASE, TEAM_2, &scoreboard);
            main_state = UPDATE_BOARD;
            break;
          case DEC_SCORE_T1:
            updateScore(DECREASE, TEAM_1, &scoreboard);
            main_state = UPDATE_BOARD;
            break;
          case DEC_SCORE_T2:
            updateScore(DECREASE, TEAM_2, &scoreboard);
            main_state = UPDATE_BOARD;
            break;
          case START_TIMER:
            startTimer(&timerState);
            main_state = WAITING_COMMAND;
            break;
          case STOP_TIMER:
            stopTimer(&timerState);
            main_state = WAITING_COMMAND;
            break;
          case RESET_ALL:
            // Lleva todo el tablero a valores de inicio y frena el timer
            setDefaultValues(&scoreboard, &timerState);
            main_state = UPDATE_BOARD;
            break;
          default:
            command = NOTHING_TO_DO;
            break;
        }
        command = NOTHING_TO_DO;
        break;
      case UPDATE_TIMER:
        updateTimer(&scoreboard, &timerState);
        main_state = UPDATE_BOARD;
        break;
      case UPDATE_BOARD:
        setDataFrame(&scoreboard, dataFrame);                       // setear la trama de datos a enviar
        DATA_FRAME_BYTES = setBufferTx((char*)bufferTx, dataFrame); // setear buffer para enviar por serial
        Serial.write(bufferTx, DATA_FRAME_BYTES);                   // enviar data
        main_state = WAITING_COMMAND;
        break;
      case ERROR:
        Serial.println("Error: not known command");
        main_state = WAITING_COMMAND;
        break;
      default:
        main_state = WAITING_COMMAND;
        break;
    }
    delay(10);
  }
}