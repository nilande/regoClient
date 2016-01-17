#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>

#include <serialio.h>

main()
{
	char *portname = "/dev/ttyACM0";
	int fd = open(portname, O_RDWR | O_NOCTTY | O_SYNC);
	if (fd < 0) {
		fprintf(stderr, "Error %d opening %s: %s", errno, portname, strerror(errno));
		exit(1);
	}
	setSerialParams(fd);

	int n = receivePacket(fd);

	printf("%d bytes received\n", n);

	close(fd);
}
