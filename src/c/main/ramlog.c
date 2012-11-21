// ramlog.c
//
// Sprintfing debug info into a circular ram buffer.
//
// See also:
//
//     SYSCALL_LOG stuff in   src/c/h/runtime-base.h

#include "../mythryl-config.h"

#include <stdio.h>
#include <stdarg.h>
#include "runtime-base.h"

#define ONE_K 1024
#define RAMLOG_BUFFER_SIZE_IN_BYTES  (64*ONE_K)
#define RAMLOG_OVERRUN_SIZE_IN_BYTES  (4*ONE_K)

// The circular ramlog buffer.  It consists of the bytes from
// ramlog_buf[0] to ramlog_buf[ RAMLOG_BUFFER_SIZE_IN_BYTES-1 ];
// It holds a sequence of lines ended by '\n'.
// The last (most recent) one will be ended by "\n\0"
// after which there will be in general a partly-overwritten line
// before the start of the oldest remaining line in the buffer.
// With the exception of this partial line, any parts of the buffer
// not containing valid lines should contain all nulls.
//
// ramlog_next points to the next place to write a line into ramlog_buf[];
// it should always point to a null char ('\0').  The only other nulls in
// ramlog_buf[] should be a contiguous sequence of them ending at ramlog_soft_limit.
// 
//
static char  ramlog_buf [ RAMLOG_BUFFER_SIZE_IN_BYTES + RAMLOG_OVERRUN_SIZE_IN_BYTES ];
static char* ramlog_next = ramlog_buf;
static char* ramlog_soft_limit = ramlog_buf + RAMLOG_BUFFER_SIZE_IN_BYTES;
static char* ramlog_hard_limit = ramlog_buf + RAMLOG_BUFFER_SIZE_IN_BYTES  + RAMLOG_OVERRUN_SIZE_IN_BYTES;
static int   ramlog_lines_printed = 0;

// Append a one-line message to ramlog_buf.
// If it runs past ramlog_soft_limit, null it out
// and write it at start of rablog_buf:
//
void   ramlog_printf   (char *format, ...)   {
    // =============
    //
    if (!syscall_log_and_ramlog_enabled)   return;

    Hostthread* hostthread = pth__get_hostthread_by_ptid( pth__get_hostthread_ptid() );

    va_list   ap;

    va_start (ap, format);
    char* start_of_line = ramlog_next;
    ramlog_next +=  sprintf (ramlog_next, "%d: [%d:%s]: ", ++ramlog_lines_printed, hostthread->id, hostthread->name);
    ramlog_next += vsprintf (ramlog_next, format, ap);
    va_end(ap);

    // 'format' is supposed to end with a newline
    // but I'm forgetful, so add one if needed:
    // 
    if (ramlog_next[-1] != '\n') {
	*ramlog_next++ = '\n';
	*ramlog_next   = '\0';
    }

    if (ramlog_next >= ramlog_hard_limit) {
	//
	die("ramlog_printf: Overran buffer by %d, increase RAMLOG_OVERRUN_SIZE_IN_BYTES", ramlog_next - ramlog_hard_limit);
    }

    if (ramlog_next >= ramlog_soft_limit) {				// Line ran over end of buffer; null it out and try again at start of buffer.
	//
	char*  p  = start_of_line;
        while (p <= ramlog_soft_limit) *p++  = '\0';			// Null out first try.
	//
        va_start (ap, format);
	ramlog_next  = ramlog_buf;
        ramlog_next +=  sprintf (ramlog_next, "%d: [%d:%s]: ", ramlog_lines_printed, hostthread->id, hostthread->name);
        ramlog_next += vsprintf (ramlog_next, format, ap);
        va_end(ap);

	if (ramlog_next >= ramlog_soft_limit) {				// Line length > buffer length ?! =8-o
	    //
	    die("ramlog_printf: Overran buffer by %d, increase RAMLOG_OVERRUN_SIZE_IN_BYTES", ramlog_next - ramlog_soft_limit);
	}
    }
}

static int   count_lines_in_ramlog   (void) {
    //       =====================

    int newlines_seen = 0;
    int i;
    for (i = 0; i < RAMLOG_BUFFER_SIZE_IN_BYTES; ++i) {
        if (ramlog_buf[i] == '\n')  ++ newlines_seen;
    }
    if (!newlines_seen)  return 0;
    else                 return  newlines_seen -1;	// -1 for the presumed partial line.  If all lines are the same length, we'll be low by one. No biggie.
}


// Call do_one_line(arg,line_number,line) on
// every line in ramlog, oldest line first.
//
// Note that 'line' is '\n' terminated
// but NOT '\0' terminated!
//
static void   for_all_lines_in_ramlog   (void (*do_one_line)(void* arg, int line_number, char* line), void* arg) {
    //        =======================
    //
    int line_number = 0;

    char*  p = ramlog_next+1;							// Step over nul that ramlog_next points to.
    while (*p && *p != '\n' && p < ramlog_soft_limit)   ++p;			// Step over partially-overwritten line.

    while (p < ramlog_soft_limit) {						// Handle all lines between ramlog_next and ramlog_soft_limit.
	//
        if (!*p)   break;
        do_one_line(arg,line_number,p);
	while (*p && *p != '\n') ++p;						// Find end-of-line newline.
	if (!*p)   break;
	++p;									// Step over end-of-line newline.
	++line_number;
    }

    p = ramlog_buf;

    while (p < ramlog_next) {							// Handle all lines between ramlog_buf and ramlog_next.
	//
        if (!*p)   break;
        do_one_line(arg,line_number,p);
	while (*p && *p != '\n') ++p;						// Find end-of-line newline.
	if (!*p)   break;
        ++p;									// Step over end-of-line newline.
	++line_number;
    }
}

static void   maybe_print_line   (void* arg, int line_number, char* line) {
    //        ================
    int first_line_to_print = (int) arg;					// Ah, type system of C -- how can we not love thee?  Let us count the ways!  :-)

    if (line_number < first_line_to_print)   return;

    while (line >= &ramlog_buf[ 0                           ]
    &&     line <  &ramlog_buf[ RAMLOG_BUFFER_SIZE_IN_BYTES ]
    &&     *line != '\n'
    &&     *line != '\0'
    ){
        putchar(*line++);
    }
    putchar('\n');
}

static void   write_line_to_file   (void* arg, int line_number, char* line) {
    //        ==================
    FILE* fd = (FILE*) arg;

    fprintf(fd,"%d: ",line_number);

    while (line >= &ramlog_buf[ 0                           ]
    &&     line <  &ramlog_buf[ RAMLOG_BUFFER_SIZE_IN_BYTES ]
    &&     *line != '\n'
    &&     *line != '\0'
    ){
        fputc( *line++, fd );
    };
    fputc( '\n', fd );
}

// Print last 'n' lines in ramlog:
//
void   debug_ramlog  (int lines_to_print) {
    // ============

    int lines_available =  count_lines_in_ramlog ();

    printf("ramlog contains %d lines\n", lines_available);

    if (!lines_available)   return;

    if (lines_to_print > lines_available) {
        lines_to_print = lines_available;
    }

    int first_line_to_print = lines_available - lines_to_print;

    for_all_lines_in_ramlog( maybe_print_line, (void*) first_line_to_print );
}


void   dump_ramlog__guts   (FILE* fd)   {
    // =================

    for_all_lines_in_ramlog(  write_line_to_file,  (void*) fd  );
} 


// Code by Jeff Prothero Copyright (c) 2010-2012,
// released under Gnu Public Licence version 3.


