#!/bin/sh

	read ans </dev/tty


	echo "1 $ans"
	echo "2 $ans" 1>&2

	exit 0
