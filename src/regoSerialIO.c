/*
 * regoSerialIO.c
 *
 * Serial communications library for the Rego637 heatpump controller
 */

#include <errno.h>			/* For error handling */
#include <fcntl.h>			/* For serial port locking */
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>		/* For setting non-canonical I/O mode */

#include <regoSerialIO.h>
#include <regoComm.h>

/*****************************************************************************
 * Defines
 *****************************************************************************/

// Buffer settings
#define REGO_COM_BUF_SIZE	43

/*****************************************************************************
 * Shared variables
 *****************************************************************************/

// Packet buffer for the rego communications both send and receive
struct {
  char buffer[REGO_COM_BUF_SIZE];
  uint8_t len;
} regoPacket;

// File descriptor for the serial port
int fd;

/*****************************************************************************
 * Internal function declarations
 *****************************************************************************/

int16_t decodeInt(char* buffer);
void encodeInt(char* buffer, int16_t number);
uint8_t decodeText(char* buffer, char* text);
char checksum(char* buffer, uint8_t len);

/*****************************************************************************
 * Functions
 *****************************************************************************/

/*
 * Open serial port
 */
void openSerialPort() {
  char *portname = PORT_NAME;
  fd = open(portname, O_RDWR | O_NOCTTY | O_SYNC);
  if (fd < 0) {
    perror("openSerialPort: error opening port");
    exit(EXIT_FAILURE);
  }

  // Make sure we close port on program termination
  atexit(closeSerialPort);

  // Set serial parameters
  setSerialParams();
}

/*
 * Close serial port
 */
void closeSerialPort() {
  close(fd);
}

/*
 * Set serial parameters
 */
int setSerialParams() {
	struct termios settings;
	struct flock fl;

	if (tcgetattr(fd, &settings) < 0) {
		perror("setSerialParams: error in tcgetattr");
	    exit(EXIT_FAILURE);
	}

	//cfsetispeed(&settings, B19200);	// Set 19200 baud input rate. Not needed for ACM device?
	//cfsetospeed(&settings, B19200);	// Set 19200 baud output rate. Not needed for ACM device?

	/* Enable raw mode (non-canonical mode, No echo, etc. See man cfmakeraw()) */
	settings.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
	settings.c_oflag &= ~OPOST;
	settings.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
	settings.c_cflag &= ~(CSIZE | PARENB);
	settings.c_cflag |= CS8;

	settings.c_cc[VMIN] = 42;					// Largest datagram is 42 characters (in read)
	settings.c_cc[VTIME] = 1;					// Wait 1/10 second before returning

	if (tcsetattr(fd, TCSANOW, &settings) < 0) {
		perror("setSerialParams: error in tcsetattr");
	    exit(EXIT_FAILURE);
	}

	/* Set a write lock on the serial port. If the port is locked, wait for access */
	fl.l_type = F_WRLCK; 		/* Write lock */
	fl.l_whence = SEEK_SET; 	/* SEEK_SET, SEEK_CUR, SEEK_END */
	fl.l_start = 0;        		/* Offset from l_whence         */
	fl.l_len = 0;        		/* length, 0 = to EOF           */
	fl.l_pid = getpid(); 		/* our PID                      */

	/* Implement a waiting lock. Exit if interrupted */
	if (fcntl(fd, F_SETLKW, &fl) < 0) {
		perror("setSerialParams: fcntl did not acquire lock");
	    exit(EXIT_FAILURE);
	}

	return 0;
}

/*
 * Decode a 16-bt integer from the buffer at the location referenced by the pointer
 */
int16_t decodeInt(char* buffer) {
	return (buffer[0] << 14) | (buffer[1] << 7) | buffer[2];
}

/*
 * Encode a 16-bit integer onto the buffer at the location referenced by the pointer
 */
void encodeInt(char* buffer, int16_t number) {
	buffer[2] = number & 0x7f;
	number >>= 7;
	buffer[1] = number & 0x7f;
	number >>= 7;
	buffer[0] = number;
}

/*
 * Decode 20 characters from a 40-byte packet into a text buffer
 * Returns the byte count including newline and null terminator
 * This needs to be used! 20 characters _may_ take up more than 22 bytes!
 */
uint8_t decodeText(char* buffer, char* text) {
	uint8_t i, j = 0;
	unsigned char temp;

	// Extract the 20 chars from the buffer
	for (i = 0; i < 40; i+=2) {
		text[j] = buffer[i] << 4 | buffer[i+1];

		// Remap certain characters from LCD to UTF-8
		switch (text[j]) {
			case 'J':
				text[j] = 'M';			
				break;

			case 'j':
				text[j] = 'm';
				break;
		}

		// *Magic* to convert high chars from ISO-8859-1 (single-byte)
		// to UTF-8 (double-byte)
		if (text[j] & 0x80) {
			temp = text[j];
			text[j++] = 0xc2 + (temp > 0xbf);
			text[j] = (temp & 0x3f) + 0x80;
		}

		j++;
	}

	// Terminate with newline and null character
	text[j++] = '\n';
	text[j++] = 0;
	return j;
}

/*
 * Calculate XOR checksum of the buffer
 */
char checksum(char* buffer, uint8_t len) {
	char checksum = 0;
	uint8_t i;
	for (i = 0; i < len; i++) {
		checksum ^= buffer[i];
	}
	return checksum;
}

/*
 * Build a 9 byte TX packet
 * Return value is packet length
 */
uint8_t buildPacket(uint8_t device, uint8_t command, uint16_t reg, uint16_t data) {
	regoPacket.buffer[0] = device;
	regoPacket.buffer[1] = command;
	encodeInt(regoPacket.buffer+2, reg);
	encodeInt(regoPacket.buffer+5, data);
	regoPacket.buffer[8] = checksum(regoPacket.buffer+2, 6);

	regoPacket.len = 9;
	return regoPacket.len;
}

/*
 * Pretty print a packet
 */
void prettyPrintPacket() {
	uint8_t i;
	printf("[ ");
	for (i = 0; i < regoPacket.len; i++) {
		if ((i % 10 == 0) && (i > 0)) printf("\n  ");
		printf("%02x ", regoPacket.buffer[i] & 0xff);
	}
	printf("]\n");
}

/*
 * Receive packet into buffer
 */
uint8_t receivePacket() {
  regoPacket.len = read(fd, regoPacket.buffer, REGO_COM_BUF_SIZE);
  return regoPacket.len;
}

/*
 * Send packet already in buffer
 */
void sendPacket() {
	write(fd, regoPacket.buffer, regoPacket.len);
}

/*
 * Decode a received response packet containing an integer
 */
int8_t decodeIntPacket(int16_t* value) {
	if (regoPacket.len == 0) {
		return RESPONSE_TIMEOUT;
	} else if (regoPacket.len != 5) {
		return RESPONSE_INVALID_LENGTH;
	}
	if (regoPacket.buffer[0] != DEVICE_ME) {
		return RESPONSE_INVALID_ADDRESS;
	}
	if (checksum(regoPacket.buffer+1, 3) != regoPacket.buffer[4]) {
		if (ignoreChecksumsFlag) {
			printf("Checksum error: %02x != %02x\n", checksum(regoPacket.buffer+1, 3), regoPacket.buffer[4]);
		} else {
			return RESPONSE_CHECKSUM_ERROR;
		}
	}

	// Packet is valid, decode data
	*value = decodeInt(regoPacket.buffer+1);
	return RESPONSE_OK;
}

/*
 * Decode received display packet
 * len = returned number of bytes
 * text = char array to decode into 
 */
int8_t decodeDisplayPacket(uint8_t* len, char* text) {
	if (regoPacket.len == 0) {
		return RESPONSE_TIMEOUT;
	} else if (regoPacket.len != 42) {
		return RESPONSE_INVALID_LENGTH;
	}
	if (regoPacket.buffer[0] != DEVICE_ME) {
		return RESPONSE_INVALID_ADDRESS;
	}
	if (checksum(regoPacket.buffer+1, 40) != regoPacket.buffer[41]) {
		if (ignoreChecksumsFlag) {
			printf("Checksum error: %02x != %02x\n", checksum(regoPacket.buffer+1, 40), regoPacket.buffer[41]);
		} else {
			return RESPONSE_CHECKSUM_ERROR;
		}
	}

	// Packet is valid, decode data
	*len = decodeText(regoPacket.buffer+1, text);
	return RESPONSE_OK;
}

