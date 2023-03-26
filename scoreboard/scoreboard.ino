/***************************************************************************/
/******************************** DEFINES **********************************/

#define DATA_FRAME_ROWS 13      // Filas de la matriz dataFrame a enviar a placa controladora. Filas -> header, comando, dato, ...
#define DATA_FRAME_COLUMNS 2    // Columnas de la matriz dataFrame a enviar a placa controladora (2 columnas -> pares [key, value])
#define DECREASE 0
#define INCREASE 1
#define TEAM1 0
#define TEAM2 1

/***************************************************************************/
/****************************** DATA TYPES *********************************/

struct scoreboard_t{
  int SCORE[2] = { 0, 0 };
} scoreboard;

enum main_states_t
{
  WAITING_COMMAND,
  UPDATE_BOARD,
  INC_SCORE_T1,
  INC_SCORE_T2,
  DEC_SCORE_T1,
  DEC_SCORE_T2,
  INC_MATCH,
  DEC_MATCH,
  STOP_TIMER,
  START_TIMER,
  RESET_TIMER,
  RESET_ALL,
  INIT
} main_state = INIT;

enum data_frame_keys_t
{
    HEADER,
    COMMAND,
    ADDRESS,
    RESPONSE,
    RESERVED_1,
    FLASH,
    RESERVED_2,
    RESERVED_3,
    SCORE_TEAM1_DECENA,
    SCORE_TEAM1_UNIDAD,
    DATA_END,
    CHECKSUM,
    FRAME_END
};

/***************************************************************************/
/******************************** FUNCTIONS ********************************/

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  delay(100);
}

/*int genChecksum(char dataFrame[DATA_FRAME_ROWS][DATA_FRAME_COLUMNS]) {
  int checksum = 0, i;
  // Para el checksum, se suman los bytes de dataFrame desde el byte COMMAND hastas DATA_END inclusive
  for(i=COMMAND; i <= DATA_END, i++){
    checksum = dataFrame[i][1]  // Los datos se encuentran en la 2da columna -> pares [key, value]
  }
  return (checksum & 0xFF); // el checksum es byte menos significativo
}*/

unsigned int setBufferTx(char* bufferTx, char dataFrame[DATA_FRAME_ROWS][DATA_FRAME_COLUMNS]) {
  char i = 0;
  unsigned int bytes_to_transfer = 0;
  for(i=0; i < DATA_FRAME_ROWS; i++){
    bufferTx[i] = dataFrame[i][1];  // Los datos se encuentran en la 2da columna -> pares [key, value]
    bytes_to_transfer++;
  }
  return bytes_to_transfer;
}

main_states_t processCommand(String bufferRx){
  String command = bufferRx;
  if(command == "INC_SCORE_T1"){ return INC_SCORE_T1; }
  else if(command == "INC_SCORE_T2"){ return INC_SCORE_T2; }
  else if(command == "DEC_SCORE_T1"){ return DEC_SCORE_T1; }
  else if(command == "DEC_SCORE_T2"){ return DEC_SCORE_T2; }
  else if(command == "INC_MATCH"){ return INC_MATCH; }
  else if(command == "DEC_MATCH"){ return DEC_MATCH; }
  else if(command == "STOP_TIMER"){ return STOP_TIMER; }
  else if(command == "START_TIMER"){ return START_TIMER; }
  else if(command == "RESET_TIMER"){ return RESET_TIMER; }
  else if(command == "RESET_ALL"){ return RESET_ALL; }
  else{
    return WAITING_COMMAND;
  }
}

void updateScore(int increaseScore, int team, scoreboard_t* scoreboard){
  if(increaseScore == INCREASE){
    if(scoreboard->SCORE[team] < 99) { scoreboard->SCORE[team]++; };
  }
  else{
    if(scoreboard->SCORE[team] > 0) { scoreboard->SCORE[team]--; };
  }
}

/* ----------------------------------------------------------------------------------------------------------------------------------------------------- */
void loop() {
  String bufferRx;
  char bufferTx[50];

  char dataFrame[DATA_FRAME_ROWS][DATA_FRAME_COLUMNS] = {
    { HEADER, 0x7F },
    { COMMAND, 0xDD },            // enviar data
    { ADDRESS, 0x00 },            // broadcast
    { RESPONSE, 0x01 },           // no return code
    { RESERVED_1, 0x00 },
    { FLASH, 0x01 },              // no guardar en flash
    { RESERVED_2, 0x00 },
    { RESERVED_3, 0x00 },
    { SCORE_TEAM1_DECENA, 0x30 },
    { SCORE_TEAM1_UNIDAD, 0x30 },
    { DATA_END, 0xFF },
    { CHECKSUM, 0x3E },
    { FRAME_END, 0x7F },
  };
  unsigned int SCOREBOARD_CMD_BYTES = 0;

  /****************************** STATE MACHINE LOOP *******************************/
  while(1){
    switch(main_state){
      case WAITING_COMMAND:
        /************************** WAIT FOR NEW COMMANDS **************************/
        Serial.println("Waiting command...");
        while (Serial.available() == 0) {}  // a la espera de datos por uart
        bufferRx = Serial.readString();
        bufferRx.trim();
        main_state = processCommand(bufferRx);
        Serial.print(bufferRx);             // enviar a controlador LED
        
        Serial.print("\nReceived: ");
        Serial.println(main_state);
        break;
      case INC_SCORE_T1:
        updateScore(INCREASE, TEAM1, &scoreboard);

        Serial.print("Score TEAM 1: ");
        Serial.println(scoreboard.SCORE[TEAM1]);
        Serial.print("Score TEAM 2: ");
        Serial.println(scoreboard.SCORE[TEAM2]);

        main_state = UPDATE_BOARD;
        break;
      case UPDATE_BOARD:
        SCOREBOARD_CMD_BYTES = setBufferTx((char*)bufferTx, dataFrame);  // setear la trama de datos a enviar
        Serial.write(bufferTx, SCOREBOARD_CMD_BYTES);                    // enviar data
        main_state = WAITING_COMMAND;
        break;
      case INIT:
        /******************************* INIT BOARD ********************************/
        SCOREBOARD_CMD_BYTES = setBufferTx((char*)bufferTx, dataFrame);  // setear la trama de datos a enviar
        Serial.write(bufferTx, SCOREBOARD_CMD_BYTES);                      // enviar data
        main_state = WAITING_COMMAND;
        break;
      default:
        main_state = WAITING_COMMAND;
        break;
    }
    delay(10);
  }
}