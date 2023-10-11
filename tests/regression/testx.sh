#!/bin/sh

	TMPFILE=/tmp/reversedfilelist.$$

	tar cf - . | 
	cpio -it | 
	awk '{print NR, $0}' | 
	sort -rn | 
	sed -e 's/^[^ ]* //' >$TMPFILE

	tar cf /tmp/xx.tar .
	
	#cat $TMPFILE
	#../../swsupplib/tests/testvarfsseek2 </tmp/xx.tar
	../../swsupplib/tests/testvarfsseek2 -f $TMPFILE </tmp/xx.tar 1>/dev/null

	#rm /tmp/xx.tar
	#stat test012.sh | grep -v ^Device | grep -v '^Access: [A-Z]' | sed -e 's/\(  Size: [^ ]*\).*/\1/'

	#cat $TMPFILE
	rm -f $TMPFILE

