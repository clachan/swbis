/*
=!  MODIFIED by jhlowe for swbis  2004-01-20
*/

/*
 * Copyright (c) 2003 Nils Nordman.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "includes.h"

#include "progressmeter.h"
#include "atomicio.h"
/*#include "misc.h"*/

#define DEFAULT_WINSIZE 80
#define MAX_WINSIZE 512
#define PADDING 1		/* padding between the progress indicators */
#define UPDATE_INTERVAL 1	/* update the progress meter every second */
#define STALL_TIME 3		/* we're stalled after this many seconds */


size_t strlcat(char *dst, const char *src, size_t siz);


/* determines whether we can output to the terminal */
static int can_output(void);

/* formats and inserts the specified size into the given buffer */
static void format_size(char *, int, uintmax_t);
static void format_rate(char *, int, uintmax_t);

/* updates the progressmeter to reflect the current state of the transfer */
void refresh_progress_meter(void);

/* signal handler for updating the progress meter */
/* jhlowe MODIFIED for swbis 2003-11-20 */ /* static void update_progress_meter(int); */

static time_t start; 		/* start progress */
static time_t of_start; 
static time_t last_update; 	/* last progress update */
static char *file; 		/* name of the file being transferred */
static uintmax_t end_pos = 0; 	/* ending position of transfer */
static uintmax_t overcount = 0; 	/* transfer position as of last refresh */
static uintmax_t over_pos = 0; 
static uintmax_t cur_pos = 0; 	/* transfer position as of last refresh */
static volatile uintmax_t *counter;	/* progress counter */
static long stalled; 		/* how long we have been stalled */
static uintmax_t bytes_per_second; 	/* current speed in bytes per second */
static int win_size; 		/* terminal window size */
static uintmax_t oldcur_speed;
static uintmax_t avg_speed = 0;
static int g_ofd = STDOUT_FILENO;

/* units for format_size */
static const char unit[] = " KMGT";

static int
can_output(void)
{
	return (getpgrp() == tcgetpgrp(g_ofd));
}

static void
format_rate(char *buf, int size, uintmax_t bytes)
{
	unsigned long dp;
	int i;

	for (i = 0; bytes >= 10000 && unit[i] != 'T'; i++)
		bytes = bytes / 1024;

	dp = (unsigned long)(bytes);
	snprintf(buf, size, "%5lu%c%s",
	    dp,
	    unit[i],
	    i ? "B" : "B");
}

static void
format_size(char *buf, int size, uintmax_t bytes)
{
	int i, j;
	uintmax_t obytes;
	unsigned long dp;

	if (over_pos) {
		obytes = over_pos;
		for (j = 0; obytes >= 10000 && unit[j] != 'T'; j++)
			obytes = (obytes) / 1024;
		for (i = 0; i < j; i++)
			bytes = (bytes) / 1024;
	} else {
		obytes = 0;
		for (i = 0; bytes >= 10000 && unit[i] != 'T'; i++)
			bytes = (bytes) / 1024;
	}
	bytes += obytes;
	dp = (unsigned long)(bytes);
	snprintf(buf, size, "%4lu%c%s",
	    dp,
	    unit[i],
	    i ? "B" : "B");
}

void
set_progress_meter_fd(int fd)
{
	g_ofd = fd;
}

int
get_progress_meter_fd(void)
{
	return g_ofd;
}

void
refresh_progress_meter(void)
{
	char buf[MAX_WINSIZE + 1];
	time_t now;
	uintmax_t newtransferred;
	uintmax_t transferred;
	double elapsed;
	double of_elapsed;
	double newelapsed;
	int percent;
	uintmax_t cur_speed;
	int hours, minutes, seconds;
	int i, len;
	int file_len;

	now = time(NULL);
	if (*counter < cur_pos) {
		over_pos=cur_pos;
		overcount++;
		of_start = now;
	}

	transferred = *counter;
	newtransferred = *counter - cur_pos;
	cur_pos = *counter;

	newelapsed = now - last_update;
	elapsed = now - start;
	of_elapsed = now - of_start;

	/* calculate speed */
	if (newelapsed < 1 || of_elapsed < 1) {
		cur_speed = oldcur_speed;
		avg_speed = cur_speed;
	} else {
		cur_speed = newtransferred / newelapsed;
		avg_speed = transferred / of_elapsed;
	}
	oldcur_speed = cur_speed;
	bytes_per_second = cur_speed;

	/* filename */
	buf[0] = '\0';
	file_len = win_size;
	if (file_len > 0) {
		len = snprintf(buf, file_len + 1, "\r%s", file);
		if (len < 0)
			len = 0;
		for (i = len;  i < file_len; i++ )
			buf[i] = ' ';
		buf[file_len] = '\0';
	}
	file_len = win_size - 54;
	buf[file_len] = '\0';

	/* percent of transfer done */
	if (end_pos != 0) {
		percent = ((float)cur_pos / end_pos) * 100;
		snprintf(buf + strlen(buf), win_size - strlen(buf),
		    " %3d%% ", percent);
	} else {
		percent = 100;
		snprintf(buf + strlen(buf), win_size - strlen(buf),
		    " ---%% ");
	}

	/* amount transferred */
	format_size(buf + strlen(buf), win_size - strlen(buf),
	    cur_pos);
	strlcat(buf, " @ ", win_size);
	
	/* Avg speed */
	format_rate(buf + strlen(buf), win_size - strlen(buf),
	    avg_speed);
	strlcat(buf, "/s[Avg]  ", win_size);

	/* bandwidth usage */
	if (bytes_per_second < 10)
		stalled++;
	else
		stalled = 0;

	if (stalled >= STALL_TIME) {
		strlcat(buf, "- stalled -", win_size);
	} else {
		format_rate(buf + strlen(buf), win_size - strlen(buf),
		    bytes_per_second);
		strlcat(buf, "/s ", win_size);
	}

	seconds = elapsed;

	hours = seconds / 3600;
	seconds -= hours * 3600;
	minutes = seconds / 60;
	seconds -= minutes * 60;

	if (hours != 0)
		snprintf(buf + strlen(buf), win_size - strlen(buf),
		    "%d:%02d:%02d", hours, minutes, seconds);
	else
		snprintf(buf + strlen(buf), win_size - strlen(buf),
		    "  %02d:%02d", minutes, seconds);

	strlcat(buf, "    ", win_size);

	atomicio(vwrite, g_ofd, buf, win_size);
	last_update = now;
}

void
update_progress_meter(int ignore)
{
	int save_errno;

	save_errno = errno;

	if (can_output())
		refresh_progress_meter();

	signal(SIGALRM, update_progress_meter);
	alarm(UPDATE_INTERVAL);
	errno = save_errno;
}

void
start_progress_meter(int ofd, char *f, off_t filesize, uintmax_t *stat)
{
	struct winsize winsize;

	start = last_update = time(NULL);
	of_start = start;
	file = f;
	end_pos = (uintmax_t)(filesize);
	cur_pos = 0;
	counter = stat;
	stalled = 0;
	g_ofd = ofd;

	if (ioctl(g_ofd, TIOCGWINSZ, &winsize) != -1 &&
	    winsize.ws_col != 0) {
		if (winsize.ws_col > MAX_WINSIZE)
			win_size = MAX_WINSIZE;
		else
			win_size = winsize.ws_col;
	} else
		win_size = DEFAULT_WINSIZE;
	win_size += 1;					/* trailing \0 */

	if (can_output())
		refresh_progress_meter();

	signal(SIGALRM, update_progress_meter);
	alarm(UPDATE_INTERVAL);
}

void
stop_progress_meter(void)
{
	alarm(0);

	if (!can_output())
		return;

	/* Ensure we complete the progress */
	
	if (cur_pos != end_pos || end_pos == 0)
		refresh_progress_meter();
	atomicio(vwrite, g_ofd, "\n", 1);
}
