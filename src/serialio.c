#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <sys/time.h>

#include <serialio.h>

/*************************************************************************************
 * Defines
 *************************************************************************************/

// Serial timeouts in milliseconds
#define REGO_RX_RESPONSE_TIMEOUT 1000;
#define REGO_RX_CHAR_TIMEOUT 50;

// Buffer settings
#define REGO_COM_BUF_SIZE 43

/*************************************************************************************
 * Shared variables
 *************************************************************************************/

char regoComBuffer[REGO_COM_BUF_SIZE];

/*************************************************************************************
 * Functions
 *************************************************************************************/

int setSerialParams(int fd) {
  struct termios tty;
  memset (&tty, 0, sizeof tty);

  // Error handling
  if (tcgetattr (fd, &tty) != 0) {
    fprintf(stderr, "Error %d from tcgetattr", errno);
    return -1;
  }

  // Set baud rate
  cfsetospeed (&tty, B115200);
  cfsetispeed (&tty, B115200);

  // Port settings
  tty.c_cflag &= ~PARENB;		// Set to 8N1
  tty.c_cflag &= ~CSTOPB;
  tty.c_cflag &=  ~CSIZE;
  tty.c_cflag |= CS8;			// 8-bit chars
  tty.c_cflag &= ~CRTSCTS;              // No flow control
  tty.c_cc[VMIN]  = 0;            	// No minimum return chars (non-blocking read)
  tty.c_cc[VTIME] = 0;            	// 0.0 seconds read timeout (non-blocking read)
  tty.c_cflag |= (CLOCAL | CREAD);	// ignore modem controls, enable reading

  // Flush port, apply attributes
  tcflush(fd, TCIFLUSH);
  if (tcsetattr (fd, TCSANOW, &tty) != 0) {
    fprintf(stderr, "Error %d from tcsetattr", errno);
    return -1;
  }
  return 0;
}

// Receive packet into buffer
int receivePacket(int fd) {
  struct timeval tvalStart, tvalNow, tvalDiff;
  int bytesReceived = 0;
  long diffMillis, timeoutMillis = REGO_RX_RESPONSE_TIMEOUT; 
  char buf;

  // Start timer
  gettimeofday(&tvalStart, NULL);

  // Wait for incoming data on the serial line
  do {
    // If data has been received, add to buffer
    if (read(fd, &buf, 1) == 1) {
      regoComBuffer[bytesReceived] = buf;
      bytesReceived++;

      // Reset timers
      gettimeofday(&tvalStart, NULL);
      timeoutMillis = REGO_RX_CHAR_TIMEOUT;
    }

    // Break out of the function to prevent overflow
    if (bytesReceived == REGO_COM_BUF_SIZE) {
      break;
    }

    // Calculate timer diff
    gettimeofday(&tvalNow, NULL);
    timersub(&tvalNow, &tvalStart, &tvalDiff);
    diffMillis = tvalDiff.tv_sec * 1000 + tvalDiff.tv_usec / 1000;
  } while (diffMillis < timeoutMillis);

  return bytesReceived;
}
