#!/bin/bash
# Awk program to add stdin  to the first
# blank line of "$1"
#
filename="$1"
if [ ! -f "$filename" ]; then
	echo "$filename not found" 1>&2
	exit 1
fi

awk -- '
        BEGIN {done=0; }
        {
	        if ( /^$/ ) {
			if (done == 0) {
				system("cat");
				done=1;
			} else {
				printf("\n");
			}
		} else {
			printf("%s\n", $0);
		}
	}
        END {  }
        ' "$filename"
