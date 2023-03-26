#include <stdio.h>

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
    DATA,
    DATA_END,
    CHECKSUM,
    FRAME_END
} keys;

char dataToSend[12];
char dataFrame[12][2] = {
  { HEADER, 0x7F },
  { COMMAND, 0x7D },  // Send data
  { ADDRESS, 0x00 },  // Broadcoast
  { RESPONSE, 0x01 }, // No return code
  { RESERVED_1, 0x00 },
  { FLASH, 0x01 },    // Don't save to flash
  { RESERVED_2, 0x00 },
  { RESERVED_3, 0x00 },
  { DATA, 0x30 },     // Data
  { DATA_END, 0xFF },
  { CHECKSUM, 0x0E },
  { FRAME_END, 0x00 },
};

int main()
{
    unsigned int BYTES_TO_TRANSFER = sizeof(dataToSend) / sizeof(dataToSend[0]);
    char i = 0;
    printf("BYTES_TO_TRANSFER: %u\n", BYTES_TO_TRANSFER);
    printf("dataToSend:\n");
    for(i=0; i < BYTES_TO_TRANSFER; i++){
        dataToSend[i] = dataFrame[i][1];
        printf("%x ", dataToSend[i]);
    }
    return 0;
}