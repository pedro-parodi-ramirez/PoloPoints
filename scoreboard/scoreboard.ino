/***************************************************************************/
/******************************** VARIABLES ********************************/
/***************************************************************************/
String bufferRx;
char bufferTx[100];

enum main_states_t
{
    INIT,
    WAITING_COMMAND
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
    SCORE_TEAM_1,
    DATA_END,
    CHECKSUM,
    FRAME_END
};

char dataFrame[12][2] = {
  { HEADER, 0x7F },
  { COMMAND, 0xDD },      // send data
  { ADDRESS, 0x00 },      // broadcoast
  { RESPONSE, 0x01 },     // no return code
  { RESERVED_1, 0x00 },
  { FLASH, 0x01 },        // don't save to flash
  { RESERVED_2, 0x00 },
  { RESERVED_3, 0x00 },
  { SCORE_TEAM_1, 0x30 }, // data
  { DATA_END, 0xFF },
  { CHECKSUM, 0x0E },
  { FRAME_END, 0x7F },
};
unsigned int BOARD_SIZEOF_FRAME = sizeof(dataFrame) / ( 2 * sizeof(dataFrame[0][0])); // amount of bytes to transfer -> size of dataFrame / 2 (two columns)

/***************************************************************************/
/******************************** FUNCTIONS ********************************/
/***************************************************************************/
void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  delay(100);
}

int genChecksum(int data) {
  
}

void setDataFrame() {
  char i = 0;
  for(i=0; i < BOARD_SIZEOF_FRAME; i++){
    bufferTx[i] = dataFrame[i][1];
  }
}

/* ----------------------------------------------------------------------------------------------------------------------------------------------------- */
void loop() {

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
      Serial.print("BOARD_SIZEOF_FRAME: ");
      Serial.println(BOARD_SIZEOF_FRAME);
      setDataFrame();                              // set the data frame to send
      Serial.write(bufferTx, BOARD_SIZEOF_FRAME);  // send the data
      main_state = WAITING_COMMAND;
      break;
    default:
      main_state = WAITING_COMMAND;
      break;
  }
  delay(10);
}