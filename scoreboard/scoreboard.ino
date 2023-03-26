/***************************************************************************/
/**************************** GLOBAL VARIABLES *****************************/
/***************************************************************************/
#define DATA_FRAME_ROWS 13      // Filas de la matriz dataFrame a enviar a placa controladora. Filas -> header, comando, dato, ...
#define DATA_FRAME_COLUMNS 2    // Columnas de la matriz dataFrame a enviar a placa controladora (2 columnas -> pares [key, value])

/***************************************************************************/
/******************************** FUNCTIONS ********************************/
/***************************************************************************/
void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  delay(100);
}

int genChecksum() {
  
}

unsigned int setDataFrame(char* bufferTx, char dataFrame[DATA_FRAME_ROWS][DATA_FRAME_COLUMNS]) {
  char i = 0;
  unsigned int bytes_to_transfer = 0;
  for(i=0; i < DATA_FRAME_ROWS; i++){
    bufferTx[i] = dataFrame[i][1];  // Los datos se encuentran en la 2da columna -> pares [key, value]
    bytes_to_transfer++;
  }
  return bytes_to_transfer;
}

/* ----------------------------------------------------------------------------------------------------------------------------------------------------- */
void loop() {
  String bufferRx;
  char bufferTx[100];

  enum main_states_t
  {
    WAITING_COMMAND,
    INIT
  } main_state = INIT;

  enum keys_t
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

  char dataFrame[DATA_FRAME_ROWS][DATA_FRAME_COLUMNS] = {
    { HEADER, 0x7F },
    { COMMAND, 0xDD },            // send data
    { ADDRESS, 0x00 },            // broadcoast
    { RESPONSE, 0x01 },           // no return code
    { RESERVED_1, 0x00 },
    { FLASH, 0x01 },              // don't save to flash
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
        while (Serial.available() == 0) {}  // wait for data available
        bufferRx = Serial.readString();     // read until timeout
        bufferRx.trim();

        Serial.print(bufferRx);             // send to the LED controller
        break;
      case INIT:
        /******************************* INIT BOARD ********************************/
        SCOREBOARD_CMD_BYTES = setDataFrame((char*)bufferTx, dataFrame);  // set the data frame to send
        Serial.write(bufferTx, SCOREBOARD_CMD_BYTES);                                                                  // send the data
        main_state = WAITING_COMMAND;
        break;
      default:
        main_state = WAITING_COMMAND;
        break;
    }
    delay(10);
  }
}