/* swicol.h -- Write script and data into a shell's stdin.
 *
 * Copyright (C) 2004  James H. Lowe, Jr.  <jhlowe@acm.org>
 *
 */

/*
 * COPYING TERMS AND CONDITIONS:
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 3, or (at your option)
 *  any later version.
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef swicol_h_20030523
#define swicol_h_20030523

#include "swuser_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "swlib.h"
#include "uxfio.h"
#include "strob.h"
#include "strar.h"
#include "swvarfs.h"
#include "hllist.h"
#include "defer.h"
#include "porinode.h"
#include "ahs.h"
#include "taru.h"
#include "taruib.h"
#include "uinfile.h"
#include "swheader.h"
#include "swheaderline.h"

#define SWICOL_TRAILER 	CPIO_INBAND_EOA_FILENAME
#define SWICOL_ALARM_EVENT "Intentional abort"
#define SWICOL_STATUS_NOMINAL_ERROR	129
#define SWICOL_ABORT_STEP_1 1
#define SWICOL_ABORT_STEP_2 2

#define SWICOL_DEBUG_TASK_SCRIPT_PREFIX "/tmp/swbis_task_"

#define SWICOL_TL_1	1	/* time limit in seconds */
#define SWICOL_TL_2	2
#define SWICOL_TL_3	3
#define SWICOL_TL_4	4
#define SWICOL_TL_5	5
#define SWICOL_TL_6	6
#define SWICOL_TL_7	7
#define SWICOL_TL_8	8
#define SWICOL_TL_9	9
#define SWICOL_TL_10	10
#define SWICOL_TL_11	11
#define SWICOL_TL_12	12
#define SWICOL_TL_13	13
#define SWICOL_TL_14	14
#define SWICOL_TL_15	15
#define SWICOL_TL_16	16
#define SWICOL_TL_17	17
#define SWICOL_TL_18	18
#define SWICOL_TL_19	19
#define SWICOL_TL_20	20
#define SWICOL_TL_21	21
#define SWICOL_TL_22	22
#define SWICOL_TL_23	23
#define SWICOL_TL_24	24
#define SWICOL_TL_25	25
#define SWICOL_TL_26	26
#define SWICOL_TL_27	27
#define SWICOL_TL_28	28
#define SWICOL_TL_29	29
#define SWICOL_TL_30	30
#define SWICOL_TL_31	31
#define SWICOL_TL_32	32
#define SWICOL_TL_33	33
#define SWICOL_TL_34	34
#define SWICOL_TL_40	40
#define SWICOL_TL_50	50
#define SWICOL_TL_60	60
#define SWICOL_TL_80	80
#define SWICOL_TL_100	100
#define SWICOL_TL_500	500
#define SWICOL_TL_1000	1000
#define SWICOL_TL_3600	3600
#define SWICOL_TL_6h	(SWICOL_TL_3600 * 6)
#define SWICOL_TL_12h	(SWICOL_TL_3600 * 12)
#define SWICOL_TL_1d	(SWICOL_TL_3600 * 24)
#define SWICOL_TL_UL	(SWICOL_TL_1d * 10000)

typedef struct {
	struct sw_logspec * logspecM;
	STROB * tmpM;
	STROB * scriptM;
	int verbose_levelM;
	STRAR * event_listM;
	char * setvxM;
	char * umaskM;
	char * blocksizeM;
	int delaytimeM;
	int nhopsM;
	char * targetpathM;
	int event_indexM; /* index of the last task's events in event_listM */
	STROB * id_stringM;
	int debug_task_scriptsM;
	int master_alarmM;
	int event_fdM;
	char * magic_headerM;
	int needs_synct_eoaM;
} SWICOL;


SWICOL * swicol_create(void);
void swicol_delete(SWICOL * swicol);
int swicol_rpsh_task_send_script(SWICOL * swicol, int fd, uintmax_t data_size, char * dir, char * script, char * desc);
int swicol_rpsh_task_send_script2(SWICOL * swicol, int fd, uintmax_t data_size, char * dir, char * script, char * desc);
int swicol_rpsh_task_wait(SWICOL * swicol, STROB * buf, int fd, int timelimit);
int swicol_rpsh_task_expect(SWICOL * swicol, int event_fd, int timelimit);
int swicol_rpsh_get_event_status(SWICOL * swicol, char * msg, int event_value, int first, int *found_at);
char * swicol_rpsh_get_event_message(SWICOL * swicol, int event_value, int first, int *found_at);
int swicol_show_events_to_fd(SWICOL * swicol, int fd, int first_event_index);
void swicol_set_umask(SWICOL * swicol, char * buf);
void swicol_set_setvx(SWICOL * swicol, char * buf);
void swicol_set_blocksize(SWICOL * swicol, char * buf);
void swicol_set_delaytime(SWICOL * swicol, int delaytime);
void swicol_set_verbose_level(SWICOL * swicol, int level);
char * swicol_get_umask(SWICOL * swicol);
char * swicol_get_setvxk(SWICOL * swicol);
char * swicol_get_blocksize(SWICOL * swicol);
char * swicol_subshell_marks(STROB * subsh, char * type, int wh, int nhops, int verbose_level);
char * swicol_brace_marks(STROB * subsh, char * type, int wh, int nhops, int verbose_level);
void swicol_print_event(SWICOL * swicol, STROB * buf, char * ev);
void swicol_print_events(SWICOL * swicol, STROB * buf, int first_event_index);
void swicol_set_nhops(SWICOL * swicol, int nhops);
void swicol_set_targetpath(SWICOL * swicol, char * buf);
int swicol_get_event_status(SWICOL * swicol, int event_value, int first_current_event);
void swicol_clear_task_idstring(SWICOL * swicol);
void swicol_set_task_idstring(SWICOL * swicol, char * id);
char * swicol_get_task_idstring(SWICOL * swicol);
void swicol_write_debug_task_script(SWICOL * swicol, char * script);
void swicol_set_task_debug(SWICOL * swicol, int do_debug);
void swicol_set_event_fd(SWICOL * swicol, int event_fd);
void swicol_set_master_alarm(SWICOL * swicol);
void swicol_clear_master_alarm(SWICOL * swicol);
int swicol_get_master_alarm_status(SWICOL * swicol);
int swicol_rpsh_wait_304(SWICOL * swicol, int event_fd);
int swicol_rpsh_wait_cts(SWICOL * swicol, int event_fd);
int swicol_send_loop_trailer(SWICOL * swicol, int fd);
int swicol_rpsh_wait_for_event(SWICOL * swicol, STROB * retbuf, int event_fd, int event);

#endif
