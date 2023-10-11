include ./Make.defines

PROGS =	pty
LDLIBS=../libmisc.a
CFLAGS = -I ../../.. -I ../../../include
all:	${PROGS}


pty:	main.o loop.o driver.o ptyfork.o error.o writen.o signalintr.o ptyopen.o ttymodes.o
		${LINK.c} -o pty main.o loop.o driver.o ptyfork.o error.o writen.o signalintr.o ptyopen.o ttymodes.o

clean:
	rm -f ${PROGS} ${TEMPFILES} *.o pty 
