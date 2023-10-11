#!/bin/sh
set -vx

gcc -I../../include -I.. -DSWSSH_TEST_PGM swssh.c -c && \
gcc -o swssh swssh.o ../swsupplib.a

