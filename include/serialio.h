// Serial response statuses
#define RESPONSE_OK 1
#define RESPONSE_TIMEOUT 0
#define RESPONSE_CHECKSUM_ERROR -1
#define RESPONSE_INVALID_LENGTH -2
#define RESPONSE_INVALID_ADDRESS -3

int setSerialParams(int fd);
void flushBuffer(int fd);
int receivePacket(int fd);
