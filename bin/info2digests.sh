#!/bin/sh

#
#  Add a newline after the file and control_file object keywords
#  to enable the awk program to decipher the multi-line records.
#


(
sed    -e 's/^[[:space:][:space:]]*file.*/\
file/' -e 's/^[[:space:][:space:]]*control_file.*/\
control_file/' |
awk -- '
BEGIN { RS = ""; FS="\n"; }
{
	#print $0
	if (/[ \t]*file[# \t]*/) { 
		#printf ("HERE %s\n", $0);
		path = field("path"); 
		md5sum = field("md5sum");
		type = field("type");
		if (type == "f") {
			printf("%s  %s\n", md5sum, path); 
		}
		next;
	}
}
END {  }
function field(name,	i,f) {
	#printf("NF = %d  NR = %d\n", NF, NR);
	for (i = 1; i <= NF; i++) {
		#printf ("IIII %s\n", $i);
		split($i, f, " ")
		if (f[1] == name)
		     return f[2]
        }
	printf("error: no field %s in record\n", name)  | "cat 1>&2"
}
'
) 2>/dev/null 

