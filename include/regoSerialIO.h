#include <stdint.h>

/*************************************************************************************
 * Defines
 *************************************************************************************/

// Serial port of device
#define PORT_NAME								"/dev/ttyACM0"

/*************************************************************************************
 * Function declarations
 *************************************************************************************/

void openSerialPort();
void closeSerialPort();
int setSerialParams();

uint8_t buildPacket(uint8_t device, uint8_t command, uint16_t reg, uint16_t data);
void prettyPrintPacket();
uint8_t receivePacket();
void sendPacket();
int8_t decodeIntPacket(int16_t* value);
int8_t decodeDisplayPacket(uint8_t* len, char* text);

