#include <stdint.h>

/*****************************************************************************
 * Defines
 *****************************************************************************/

// Rego constants and addresses
#define DEVICE_ME               0x01
#define DEVICE_HEATPUMP         0x81
#define COMMAND_READ_SYS_REG    0x02
#define COMMAND_READ_DISPLAY		0x20

// Serial response statuses
#define RESPONSE_OK								1
#define RESPONSE_TIMEOUT					0
#define RESPONSE_CHECKSUM_ERROR 	-1
#define RESPONSE_INVALID_LENGTH		-2
#define RESPONSE_INVALID_ADDRESS	-3

/*****************************************************************************
 * Variable declarations
 *****************************************************************************/

extern int graphiteOutputFlag;
extern int ignoreChecksumsFlag;
extern int showPacketsFlag;

/*****************************************************************************
 * Function declarations
 *****************************************************************************/

int8_t getRegisterIdByName(char* name);
int8_t getRegisterIdByAddress(uint16_t reg);
uint16_t getRegisterAddressById(int8_t id);
char* getRegisterDescriptionById(int8_t id);
char* getRegisterNameById(int8_t id);

int8_t queryRegister(uint16_t reg, int16_t* value);
int8_t queryDisplay(char* text);
int8_t printRegister(uint16_t reg);
void printKnownRegisters();
