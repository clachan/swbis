#include "swuser_config.h"
#include <stdio.h>
#include <stdlib.h>
#include "uxfio.h"

int main (int argc, char ** argv) {
	int bytesRead;
	int uxfio_fd;
	int bufferLength = 512;
	char buffer[1024];


	uxfio_fd = uxfio_opendup (STDIN_FILENO, O_RDONLY);
	if (uxfio_fd < 0) exit (2);
 
	if (uxfio_fcntl (uxfio_fd, UXFIO_F_SET_BUFTYPE, UXFIO_BUFTYPE_MEM) < 0 ) {
		fprintf (stderr,"error in uxfio_fcntl UXFIO_F_SET_BUFTYPE MEM  \n");
		exit (3);
	}

	if (uxfio_fcntl (uxfio_fd, UXFIO_F_SET_BUFFER_LENGTH, bufferLength) < 0 ) {
		fprintf (stderr,"error in uxfio_fcntl UXFIO_F_SETBL  \n");
		exit (4);
	}


	bytesRead = uxfio_polling_read(uxfio_fd, buffer, 512);
	if (bytesRead != 512) exit (2);
	bytesRead = 511;
	while (bytesRead == 511) {
		if (uxfio_lseek(uxfio_fd, -512 /* -512 */, SEEK_CUR) < 0) exit(5);
		bytesRead = uxfio_read(uxfio_fd, buffer, 511);
		if (write (STDOUT_FILENO, buffer, 511) != 511) {
			fprintf (stderr,"error in write\n");
			exit (6);
		}
		if (uxfio_lseek(uxfio_fd, 1, SEEK_CUR) < 0) exit(8);
		bytesRead = uxfio_polling_read(uxfio_fd, buffer, 511);
	}
	if (bytesRead < 0) exit(7);
	if (uxfio_lseek(uxfio_fd, -(bytesRead + 1), SEEK_CUR) < 0) exit(9);
	if (uxfio_polling_read(uxfio_fd, buffer, (bytesRead + 1)) != (bytesRead + 1)) exit(10);
	if (write (STDOUT_FILENO, buffer, bytesRead + 1) != (bytesRead + 1)) exit(11);

	exit(0);
}
