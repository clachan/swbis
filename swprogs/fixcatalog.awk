#! /usr/bin/awk -f 
# fixcatalog.awk -- Modify the ./catalog/ file system meta-data according to 
#                   package meta-data in ./catalog/
#
# Copyright (C) 2006 Jim Lowe
#
# COPYING TERMS AND CONDITIONS:
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3, or (at your option)
# any later version.
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301

BEGIN { 
	# Global Variables
	#
	PROGNAME="fixcatalog.awk";
	g_exitval=0;
	g_arridx=0;
	g_mode_single_line=1;

	g_swbis_dir_mode="0755";  # Built-in policy of swpackage version 0.495
	g_swbis_sd_file_mode="0644";  # Built-in policy of swpackage version 0.495
	# Attribute names
	#
	SW_A_mode="mode";
	SW_A_mtime="mtime";
	SW_A_create_time="create_time";
	SW_A_path="path";
	SW_A_dfiles="dfiles";
	SW_A_pfiles="pfiles";

	# Arrays
	#
	Lines[0]="";
	Keywords[0]="";
	Values[0]="";

	# Object Arrays
	#
	O_Installed_Software[0]="";
	O_Distribution[0]="";
	O_Bundle[0]="";
	O_Vendor[0]="";
	O_Category[0]="";
	O_Media[0]="";
	O_Product[0]="";
	O_Sub_Product[0]="";
	O_Fileset[0]="";
	O_Control_File[0]="";
	O_File[0]="";

	# Parser context variables 
	#
	in_distribution = 0; in_category = 0; in_vendor = 0;
	in_bundle = 0; in_sub_product = 0; in_product = 0; in_fileset = 0;
	in_control_file = 0; in_installed_software = 0;
	distribution_mtime=0; distribution_mode=0;

	### Program starts here
	#
	filename="catalog/INDEX"
	if (test_for_read(filename) == "") {
		errmsg(PROGNAME ": file not found: " filename);
		exit 1;
	}

	#
	# Parse the INDEX file
	#
	while (getline g_x <filename > 0) {
		parse_index(g_x);
	}; copy_object();  # This call to copy_object is required

	# debug_print_object(O_Distribution, Keywords, Values);
	# debug_print_object(O_Product, Keywords, Values);
	# debug_print_object(O_Fileset, Keywords, Values);

	#
	# Find the "create_time" and "mode" attributes in the distribution
	# object.
	#

	g_create_time = find_value(O_Distribution, SW_A_create_time);
	check_errno();
	g_mode = find_value(O_Distribution, SW_A_mode);
	check_errno();

	# dmsg("mode value is [" g_mode "]");
	# dmsg("create_time value is [" g_create_time "]");

	#
	# Find the dfiles and pfile attributes and set the defaults
	# if not present.
	#

	g_dfiles = find_value(O_Distribution, SW_A_dfiles);
	check_errno();
	if (g_found == 0) g_dfiles=SW_A_dfiles;

	g_pfiles = find_value(O_Distribution, SW_A_pfiles);
	check_errno();
	if (g_found == 0) g_pfiles=SW_A_pfiles;
	
	#
	# Find the mtimetouch program which should be located in the
	# <libexecdir> of the swbis installation
	#

	#g_mtimetouch = get_mtimetouch_path();
	if (ARGC < 2) {
		errmsg("Usage: " PROGNAME " " "<mtimetouch_program_path>");
		exit(1);
	}
	g_mtimetouch = ARGV[1];
	if (length(g_mtimetouch) == 0) {
		errmsg("unable to find the mtimetouch program from the swbis installation");
		exit(1);
	}

	#
	# Parse and correct the meta-data in the dfiles/INFO file
	#

	g_arridx=0;
	g_dirname="catalog/" g_dfiles ;
	filename=g_dirname "/INFO"
	while (getline g_x <filename > 0) {
		parse_info(g_x);
	}; process_info_object();  # This call to copy_object is required

	#
	# Parse and correct the meta-data in the pfiles/INFO file
	#

	g_arridx=0;
	g_dirname="catalog/" g_pfiles ;
	filename=g_dirname "/INFO"
	while (getline g_x <filename > 0) {
		parse_info(g_x);
	}; process_info_object();  # This call to copy_object is required

	#
	# Now correct the mtimes and modes of the INDEX file and control directories.
	#

	command="chmod" " "  g_swbis_sd_file_mode  " " "catalog/INDEX";
	retval = system(command);
	if (retval != 0) {
		errmsg("command failed: " command); exit(1);
	}

	command=g_mtimetouch " " g_create_time  " catalog/INDEX";
	retval = system(command);
	if (retval != 0) {
		errmsg("command failed: " command); exit(1);
	}
	
	command="chmod" " "  g_swbis_dir_mode  " "   "catalog/" g_dfiles " " "catalog/" g_pfiles;
	retval = system(command);
	if (retval != 0) {
		errmsg("command failed: " command); exit(1);
	}

	command = g_mtimetouch " "  g_create_time  " "    "catalog/" g_dfiles;
	retval = system(command);
	if (retval != 0) {
		errmsg("command failed: " command); exit(1);
	}

	command = g_mtimetouch " "  g_create_time  " "    "catalog/" g_pfiles;
	retval = system(command);
	if (retval != 0) {
		errmsg("command failed: " command); exit(1);
	}

	command="chmod" " "  g_swbis_sd_file_mode  " "   "catalog/INFO";
	retval = system(command);
	if (retval != 0) {
		errmsg("command failed: " command); exit(1);
	}

	command = g_mtimetouch " "  g_create_time  " "    "catalog/INFO";
	retval = system(command);
	if (retval != 0) {
		errmsg("command failed: " command); exit(1);
	}

	command="chmod" " "  g_swbis_dir_mode  " "   "catalog";
	retval = system(command);
	if (retval != 0) {
		errmsg("command failed: " command); exit(1);
	}

	command = g_mtimetouch " "  g_create_time  " "    "catalog";
	retval = system(command);
	if (retval != 0) {
		errmsg("command failed: " command); exit(1);
	}

	exit(g_exitval);
}

function parse_index(rc) {
	###
	### Pattern for when scanning for object keywords
	###
	# Sun's awk and BSD's awk do not support escaped newlines '\<newline>' in regular expression.
	if (g_mode_single_line == 1 &&  rc ~ /(^[ \t]*distribution|^[ \t]*category|^[ \t]*vendor|^[ \t]*media|^[ \t]*bundle|^[ \t]*product|^[ \t]*sub_product|^[ \t]*fileset)/) { init_index_object(rc); return;}
	###
	### Patterns for when scanning for keywords
	###
	###  Comment
	if (g_mode_single_line == 1 && rc ~ /^[\t ]*#/  ) { return; }

	###  Quoted string all on one line
	if (g_mode_single_line == 1 && rc ~ /^[ \t]*[a-zA-Z_]+[ \t]+"[^"]*"/ ) { process_keyword(rc); return; }

	###  Quoted string all on more than one line
	if (g_mode_single_line == 1 && rc ~ /^[ \t]*[a-zA-Z_]+[ \t]+"[^"]*/ ) { process_multi_keyword(rc);  return; }

	# process the intermediate line of a quoted multi-line value
	if (g_mode_single_line == 0 && rc ~ /^[^"]*$/ ) { process_value_text(rc); return; }

	# process the last line of a quoted multi-line value
	if (g_mode_single_line == 0 && rc ~ /^[^"]*"($|[ \t]*$)/ ) { process_value_text_end(rc);  return; }

	###  Any line, including single line attributes
	if (rc ~ /^[ \t]*[a-zA-Z_]/ ) { process_keyword(rc);  return; }
}

function parse_info(rc) {
	###
	### Pattern for when scanning for object keywords
	###
	# Sun's awk and BSD's awk do not support escaped newlines '\<newline>' in regular expression.
	if (g_mode_single_line == 1 &&  rc ~ /(^[ \t]*file|^[ \t]*control_file)/) { init_info_object(rc); return;}
	###
	### Patterns for when scanning for keywords
	###
	###  Comment
	if (g_mode_single_line == 1 && rc ~ /^[\t ]*#/  ) { return; }

	###  Quoted string all on one line
	if (g_mode_single_line == 1 && rc ~ /^[ \t]*[a-zA-Z_]+[ \t]+"[^"]*"/ ) { process_keyword(rc); return; }

	###  Quoted string all on more than one line
	if (g_mode_single_line == 1 && rc ~ /^[ \t]*[a-zA-Z_]+[ \t]+"[^"]*/ ) { errmsg("INFO file: invalid input");  return; }

	# process the intermediate line of a quoted multi-line value
	if (g_mode_single_line == 0 && rc ~ /^[^"]*$/ ) { errmsg("INFO file: invalid input"); return; }

	# process the last line of a quoted multi-line value
	if (g_mode_single_line == 0 && rc ~ /^[^"]*"($|[ \t]*$)/ ) { errmsg("INFO file: invalid input"); return; }

	###  Any line, including single line attributes
	if (rc ~ /^[ \t]*[a-zA-Z_]/ ) { process_keyword(rc);  return; }
}

function reset_context() {
	in_distribution = 0; in_category = 0; in_vendor = 0; in_bundle = 0;
	in_sub_product = 0; in_product = 0; in_fileset = 0;
	in_control_file = 0; in_installed_software = 0;
	g_mode_single_line=1;
}

function process_multi_keyword(line) {
	g_mode_single_line=0;
	Lines[g_arridx]=line "\n";
}

function process_value_text(line) {
	Lines[g_arridx] = Lines[g_arridx] line "\n";
}

function process_value_text_end(line) {
	g_mode_single_line=1;
	Lines[g_arridx] = Lines[g_arridx] line;
	g_arridx++;
}

function process_keyword(line) {
	g_mode_single_line=1;
	Lines[g_arridx]=line;
	g_arridx++;
}

function process_keyword2(line) {
	# printf("process_keyword line=[%s]\n", line);
	g_mode_single_line=1;
	Lines[g_arridx]=line;
	n = split(line, X);
	if (n != 2) {
		errmsg("error in split for " line );
		# exit 1;
	}
	Keywords[g_arridx]=X[1];
	Values[g_arridx]=X[2];
	g_arridx++;
}

function check_errno() {
	if (g_errno != 0) {
		errmsg("error: code=" g_errno);
	}
}

function dmsg(msg) {
	printf("%s: <debug>: %s\n", PROGNAME, msg) | "cat 1>&2"
}

function errmsg(msg) {
	printf("%s: error: %s\n", PROGNAME, msg) | "cat 1>&2"
}

function copy_object (       i) {
	if (in_distribution == 1) {
		O_Distribution[0]=Lines[0]; for(i=1; i<g_arridx; i++) { O_Distribution[i] = Lines[i]; }
	} else if (in_installed_software == 1) {
		O_Installed_Software[0]=Lines[0]; for(i=1; i<g_arridx; i++) { O_Installed_Software[i] = Lines[i]; }
	} else if (in_sub_product == 1) {
		O_Sub_Product[0]=Lines[0]; for(i=1; i<g_arridx; i++) { O_Sub_Product[i] = Lines[i]; }
	} else if (in_product == 1) {
		O_Product[0]=Lines[0]; for(i=1; i<g_arridx; i++) { O_Product[i] = Lines[i]; }
	} else if (in_bundle == 1) {
		O_Bundle[0]=Lines[0]; for(i=1; i<g_arridx; i++) { O_Bundle[i] = Lines[i]; }
	} else if (in_vendor == 1) {
		O_Vendor[0]=Lines[0]; for(i=1; i<g_arridx; i++) { O_Vendor[i] = Lines[i]; }
	} else if (in_fileset == 1) {
		O_Fileset[0]=Lines[0]; for(i=1; i<g_arridx; i++) { O_Fileset[i] = Lines[i]; }
	} else if (in_media == 1) {
		O_Media[0]=Lines[0]; for(i=1; i<g_arridx; i++) { O_Media[i] = Lines[i]; }
	} else if (in_control_file == 1) {
		O_Control_File[0]=Lines[0]; for(i=1; i<g_arridx; i++) { O_Control_File[i] = Lines[i]; }
	} else if (in_file) {
		O_File[0]=Lines[0]; for(i=1; i<g_arridx; i++) { O_File[i] = Lines[i]; }
	} else if (in_category) {
		O_Category[0]=Lines[0]; for(i=1; i<g_arridx; i++) { O_Category[i] = Lines[i]; }
	} else {
		;
		# Must be the first object in the file
	}
	return;
}

function init_info_object (line,      i) {
	#
	# Got new object, Store the one that just was completed.
	process_info_object();

	#
	# Now determine the object type from the keyword name and set the context
	# variables.

	if (line ~ /file/) {
		reset_context(); in_file = 1;
	} else if (line ~ /control_file/) {
		reset_context(); in_control_file = 1;
	} else {
		errmsg("error: unrecognized object: " line);
	}
	g_arridx=0;
	Lines[g_arridx++]=line;
}


function init_index_object (line,      i) {
	#
	# Got new object, Store the one that just was completed.
	copy_object();

	#
	# Now determine the object type from the keyword name and set the context
	# variables.

	if (line ~ /distribution/) {
		reset_context(); in_distribution = 1;
	} else if (line ~ /installed_software/) {
		reset_context(); in_installed_software = 1;
	} else if (line ~ /sub_product/) {
		reset_context(); in_sub_product = 1;
	} else if (line ~ /product/) {
		reset_context(); in_product = 1;
	} else if (line ~ /category/) {
		reset_context(); in_category = 1;
	} else if (line ~ /bundle/) {
		reset_context(); in_bundle = 1;
	} else if (line ~ /vendor/) {
		reset_context(); in_vendor = 1;
	} else if (line ~ /fileset/) {
		reset_context(); in_fileset = 1;
	} else if (line ~ /media/) {
		reset_context(); in_media = 1;
	} else if (line ~ /control_file/) {
		reset_context(); in_control_file = 1;
	} else if (line ~ /file/) {
		reset_context(); in_control_file = 1;
	} else {
		errmsg("error: unrecognized object: " line);
	}
	g_arridx=0;
	Lines[g_arridx++]=line;
}

function get_keyword(line) {
		# Remove the value part and leading whitespace
		# before the keyword
		sub(/^[ \t]*/,"", line); 
		sub(/[ \t]+.*/,"", line); 
		return line;
}

function get_value(line,     junk) {
		# Remove the keyword
		g_errno=0;
		sub(/^[ \t]*[^ \t]+[ \t]+/,"", line); 

		if ( line ~ /^[^"]+/) {
			# Unquoted Value
			# Remove the comment after the value
			sub(/#.*/,"", line); 
			sub(/[ \t]*$/,"", line); 
		} else if ( line ~ /^".*/) {
			# Quoted Value
			# Remove the First Quote
			sub(/[ \t]*"/,"", line); 
			# Remove the last Quote
			sub(/".*/,"", line); 
		} else {
			errmsg("error in get_value for [" line "]");
			line="";
			g_errno=2;
		}
		return line;
}

function find_value(O, keyword,        idx, retval, l_k) {
	idx=0;
	retval=0;
	g_found=0;
	while(length(O[idx]) > 0) {
		l_k = get_keyword(O[idx])
		if (l_k == keyword "") {
			g_found=1;
			return "" get_value(O[idx]);
		}
		idx++;
	}
	g_found=0;
	return "";
}

function parse_keyword_value(S, K, V,      l_v, l_k) {
	aa_i=0;
	while(length(S[aa_i]) > 0) {
		l_k = get_keyword(S[aa_i]);
		K[aa_i] = l_k;
		check_errno();

		l_v = get_value(S[aa_i]);
		V[aa_i] = l_v;
		check_errno();

		aa_i++;
	}
}

function debug_print_object(O, K, V) {
	parse_keyword_value(O, K, V);
	g_arridx=0;
	printf("Object=[%s]\n",  K[g_arridx++]) | "cat 1>&2";
	while(length(O[g_arridx]) > 0) {
		printf("keyword=[%s] value=[%s]\n",  K[g_arridx], V[g_arridx]) | "cat 1>&2";
		g_arridx++;
	}
}

function test_for_read(file,  junk)
{
	if ((getline junk < file) > 0) {
		close(file);
		return file;
	}
	return "";
}

function obcopy(D, S)
{
	D[0] = S[0];
	for(i=1; i<g_arridx; i++) {
		D[i] = S[i];
	}
	D[g_arridx]=""; # This add a terminating empty string
}

function process_info_object (       i, ret) {
	if (in_file == 1) {
		obcopy(O_tmp_file, Lines);
		#debug_print_object(O_tmp_file, Keywords, Values);
		ret = make_corrections(O_tmp_file);
		if (ret != 0) g_exitval=1;
	} else if (in_control_file == 1) {
		obcopy(O_tmp_file, Lines);
		#debug_print_object(O_tmp_file, Keywords, Values);
		ret = make_corrections(O_tmp_file);
		if (ret != 0) g_exitval=1;
	} else {
		;
		#
		# This code path happens for the first object.
		#
	}
	return;
}

function make_corrections(O,             v, p, mtime, att, retval) {
	retval=0;

	# Check for the terminating object which is signified by
	# and empty string
	if (length(O[0]) == 0) return 0;

	att=SW_A_mode;
	v = find_value(O, att); check_errno();
	if (g_found == 0) { errmsg(att " attribute not found"); debug_print_object(O, Keywords, Values); return 1; }

	att=SW_A_path;
	p = find_value(O, att); check_errno();
	if (g_found == 0) { errmsg(att " attribute not found"); debug_print_object(O, Keywords, Values); return 1; }

	# dmsg("Here:" "chmod" " " v " " g_dirname "/" p);
	retval = system("chmod" " " v " " g_dirname "/" p);
	if (retval != 0) { errmsg("chmod" " " v " " g_dirname "/" p); return 1; }

	att=SW_A_mtime;
	mtime = find_value(O, att); check_errno();
	if (g_found == 0) { errmsg(att " attribute not found"); return 1; }
	
	retval = system(g_mtimetouch " " mtime " " g_dirname "/" p );
	if (retval != 0) { errmsg(g_mtimetouch " " mtime " " g_dirname "/" p " failed" ); return 1; }

	return retval;
}

function file_does_exist(filename,   j) {
	j = system("ls " filename " 2>/dev/null </dev/null 1>/dev/null");
	if (j != 0) {
		return 0;
	} else {
		return 1;
	}
}

function get_mtimetouch_path(             prefix, path) {
        "swbis --help | grep bin/swpackage | sed -e 's@bin/swpackage@@'" | getline prefix;
	path=prefix "/lib/swbis/mtimetouch"
	if (file_does_exist(path) == 1) return path;

	path=prefix "/lib/swbis/mtimetouch.exe"
	if (file_does_exist(path) == 1) return path;

	path=prefix "/libexec/swbis/mtimetouch.exe"
	if (file_does_exist(path) == 1) return path;

	path=prefix "/libexec/swbis/mtimetouch"
	if (file_does_exist(path) == 1) return path;

	return "";
}
