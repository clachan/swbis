#!/bin/sh

size_fix() {

	awk 'BEGIN { oldline=""  }
	{ 
		if ($0 ~ /Size: .*/) { 
			if (oldline ~ /File: .*\/\"/) { 
				print "  Size: 0"; 
			} else {
				print $0;
			}
		} else {
			print $0;
		}
		oldline=$0	
	}
	END { }
	'

}

normalizing_filter() {
	grep -v ^Device | 
	grep -v '^Access: [A-Z]' | 
	grep -v '^Change: [A-Z]' | 
	sed -e 's/\(  Size: [^ ]*\).*/\1/'
}


	TMPFILELIST=../tmp/test019.reversedfilelist
	TMPARCHIVE=../tmp/test019.tar
	SWOUTPUT=../tmp/test019.out.1
	UOUTPUT=../tmp/test019.out.2

	#
	# Reverse the file list.
	#
	tar cf - . | 
	cpio -it | 
	awk '{print NR, $0}' | 
	sort -rn | 
	sed -e 's/^[^ ]* //' >$TMPFILELIST

	#
	# Now create the output using shell tools only.
	#
	while read file
	do
		../devel/stat "$file" | 
		normalizing_filter 
	done <"$TMPFILELIST" | size_fix >"$UOUTPUT"


	# Make an archive of this directory.
	tar cf $TMPARCHIVE .
	
	
	#
	# Now find each file in the reverse order.
	#


	echo -e \
"Testing ../../swsupplib/tests/testvarfsseek2  stdin from a reg. file  \c"
	../../swsupplib/tests/testvarfsseek2 -f "$TMPFILELIST" <"$TMPARCHIVE" |
		normalizing_filter >"$SWOUTPUT"
	#
	# Now compare the output
	#
	#cat "$UOUTPUT"
	diff -ruN "$UOUTPUT" "$SWOUTPUT"
	retval=$?
	if [ "$retval" -ne 0 ]; then
		echo "FAILED."		
		exit 1
	else
		echo "PASSED."		
	fi

	echo -e \
"Testing ../../swsupplib/tests/testvarfsseek2  from a reg. file   \c"
	../../swsupplib/tests/testvarfsseek2 -f "$TMPFILELIST" "$TMPARCHIVE" |
		normalizing_filter >"$SWOUTPUT"
	#
	# Now compare the output
	#
	#cat "$UOUTPUT"
	diff -ruN "$UOUTPUT" "$SWOUTPUT"
	retval=$?
	if [ "$retval" -ne 0 ]; then
		echo "FAILED."		
		exit 2
	else
		echo "PASSED."		
	fi


	echo -e \
"Testing ../../swsupplib/tests/testvarfsseek2  stdin from a pipe     \c"
	cat "$TMPARCHIVE" | ../../swsupplib/tests/testvarfsseek2 -f "$TMPFILELIST" |
		normalizing_filter >"$SWOUTPUT"
	#
	# Now compare the output
	#
	#cat "$UOUTPUT"
	diff -ruN "$UOUTPUT" "$SWOUTPUT"
	retval=$?
	if [ "$retval" -ne 0 ]; then
		echo "FAILED."		
		exit 3
	else
		echo "PASSED."		
	fi

	echo -e \
"Testing ../../swsupplib/tests/testvarfsseek2  from a dir file     \c"
	../../swsupplib/tests/testvarfsseek2 -f "$TMPFILELIST" ./ |
		normalizing_filter | size_fix >"$SWOUTPUT"
	#
	# Now compare the output
	#
	#cat "$UOUTPUT"
	diff -ruN "$UOUTPUT" "$SWOUTPUT"
	retval=$?
	if [ "$retval" -ne 0 ]; then
		echo "FAILED."		
		exit 4
	else
		echo "PASSED."		
	fi


	rm -f "$TMPARCHIVE"
	rm -f "$TMPFILELIST"
	rm -f "$UOUTPUT"
	rm -f "$SWOUTPUT"

	exit $retval
