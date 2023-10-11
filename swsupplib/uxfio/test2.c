#include "swuser_config.h"
#include <stdio.h>
#include <stdlib.h>
#include "uxfio.h"


/*
int
swlib_pump_amount(int discharge_fd, int suction_fd, int amount)
{
	int i = amount;
	if (swlib__pipe_pump(suction_fd, discharge_fd, (int *) (NULL), (int *) (NULL), &i)) {
		return -1;
	}
	return i;
}
*/

static int
swlib_pipe_pump(int ofd, int ifd)
{
	return 
	swlib_pump_amount(ofd, ifd, -1);
}

static int
swlib__pipe_pump(int suction_fd, int discharge_fd, int *status, int *childGotStatus, int *amount)
{
	int commandFailed = 0, pumpDead = 0, bytes, ibytes, byteswritten = 0;
	int remains;
	int c_amount = *amount;
	char buf[512];

	if (c_amount < 0) {
		do {
			bytes = taru_tape_buffered_read(suction_fd, buf, sizeof(buf));
			if (bytes < 0) {
				commandFailed = 1;	/* cavitation failure */
				pumpDead = 1;
			} else if (bytes == 0) {
				/* if it's not dead yet, it will be when we close the pipe */
				pumpDead = 1;
			} else {
				if (discharge_fd >= 0) {
					if ((bytes=uxfio_write(discharge_fd, buf, bytes)) != bytes) {
						commandFailed = 2;	/* discharge failure */
						pumpDead = 1;
					}
				}
			}
			byteswritten += bytes;
		} while (!pumpDead);
		*amount = byteswritten;
		return commandFailed;
	} else {
		remains = sizeof(buf);
		if ((c_amount - byteswritten) < remains) {
			remains = c_amount - byteswritten;
		}
		do {
			bytes = taru_tape_buffered_read(suction_fd, buf, remains);
			if (bytes < 0) {
				commandFailed = 1;	/* cavitation failure */
				pumpDead = 1;
			} else if (bytes == 0) {
				/* if it's not dead yet, it will be when we close the pipe */
				pumpDead = 1;
			} else if (bytes) {

				if (discharge_fd >= 0) {
					ibytes = uxfio_write(discharge_fd, buf, bytes);
				} else {
					ibytes = bytes;
				}

				if (ibytes != bytes) {
					commandFailed = 2;	/* discharge failure */
					pumpDead = 1;
				} else {
					byteswritten += bytes;
					if ((c_amount - byteswritten) < remains) {
						if ((remains = c_amount - byteswritten) > sizeof(buf)) {
							remains = sizeof(buf);
						}
					}
				}
			}
		} while (!pumpDead && remains);
		*amount = byteswritten;
		return commandFailed;
	}
	return -1;
}


int main(int argc, char ** argv)
{
	int nullfd = open("/dev/null", O_RDWR, 0);
	int fd1;
	int ofd;
	int fd;
	int offset;
	int amount;
	int testnumber;

/*
	fd1 = uxfio_opendup(STDIN_FILENO, UXFIO_BUFTYPE_MEM);

	ofd = open("/dev/null", O_RDWR, 0);
	fprintf(stderr, "HERE X");
	

	if (uxfio_lseek(fd1, offset, SEEK_SET) < 0) {
		fprintf(stderr, "lseek error 1\n");
		exit(2);
	}

	fprintf(stderr, "HERE A");

*/

	if (argc < 4) exit(3);

	testnumber = atoi(argv[1]);
	offset = atoi(argv[2]);
	amount = atoi(argv[3]);


	fd1 = STDIN_FILENO;
	fd = uxfio_opendup(fd1, UXFIO_BUFTYPE_DYNAMIC_MEM);
	fprintf(stderr, "HERE B fd = %d\n", fd);
	
	/* uxfio_fcntl(fd, UXFIO_F_SET_BUFTYPE, UXFIO_BUFTYPE_DYNAMIC_MEM);
	*/

	if (uxfio_lseek(fd, offset, SEEK_SET) < 0) {
		fprintf(stderr, "lseek error 1\n");
		exit(2);
	}

	if (uxfio_fcntl(fd, UXFIO_F_SET_VEOF, amount) < 0) 
		fprintf(stderr, "fd = %d\n", fd);

	/* 
	uxfio_debug_dump(fd);
	uxfio_debug_dump(fd);
	swlib_pipe_pump(STDOUT_FILENO, fd);
	*/
	
	
	/*
	swlib_pipe_pump(nullfd, fd);
	if (uxfio_lseek(fd, 0, SEEK_SET) != 0) fprintf(stderr, "error 2\n");
	if (uxfio_lseek(fd, 0, SEEK_END) != 0) fprintf(stderr, "error 3\n");
	if (uxfio_lseek(fd, 0, SEEK_SET) != 0) fprintf(stderr, "error 4k\n");
	if (uxfio_fcntl(fd, UXFIO_F_SET_VEOF, amount) < 0) fprintf(stderr, "fd = %d\n", fd);
	*/
	
	swlib_pipe_pump(STDOUT_FILENO, fd);

	uxfio_close(fd);

}
