/*  pax_commands.h -- */
/*
   Copyright (C) 2004 Jim Lowe
   All Rights Reserved.
  
   COPYING TERMS AND CONDITIONS:
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/


#include "swcommon0.h"

struct 
g_pax_write_command g_pax_write_commands[] = {
	{"pax",  
		"pax -x ustar -w"
	},
	{"star", 
		"star cbf 1 - -H ustar"
	},
	{"tar",  
		"tar cbf 1 -"
	},
	{"gtar", 
		"gtar cbf 1 - --format=pax"
	},
	{"swbistar",
		SWBISLIBEXECDIR "/swbis/swbistar"
	},
	{"detect",    /* Used by swlist, swverify */
		"false"
	},
		{(char*)NULL, (char*)NULL}
	};

struct 
g_pax_read_command g_pax_read_commands[] = {
	{"pax", 
		"pax -pe -r",
		"pax -pe -r -v",
		"pax -pe -r -k"
	},
	{"star",
		"star xpf - -U",
		"star xpvf - -U 1>&2", /* use of stdout breaks swcopy piping*/
		"star xpf - --keep-old-files 1>&2"
	},
	{"tar", 
		"tar xpf -",
		"tar xpvf - 1>&2", /*  use of stdout breaks swcopy piping. */
		"tar xpkf - 1>&2" /* use of stdout breaks swcopy piping. */
	},
	{"gtar",
		"gtar xpf - --overwrite",
		"gtar xpvf - --overwrite 1>&2",
		"gtar xpkf - 1>&2"
	},
		{(char*)NULL, (char*)NULL, (char*)NULL, (char*)NULL}
	};

struct 
g_pax_remove_command g_pax_remove_commands[] = {
	{"pax", 
		NULL,
		NULL
	},
	{"star",
		NULL,
		NULL
	},
	{"tar", 
		"tar zcf - --no-recursion --remove-files  --files-from=-",
		"tar zvcf - --no-recursion --remove-files --files-from=- 1>&2"
	},
	{"gtar",
		"gtar zcf - --no-recursion --remove-files --files-from=-",
		"gtar zvcf - --no-recursion --remove-files --files-from=- 1>&2"
	},
		{(char*)NULL, (char*)NULL, (char*)NULL}
	};


