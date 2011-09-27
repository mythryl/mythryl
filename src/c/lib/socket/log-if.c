// log-if.c
//
// See overview comments in
//
//     src/c/lib/socket/log-if.h
//
// This routine is not really socket-specific, so it
// probably belongs in some more general directory,
// but initially at least I'm using it to debug
// socket stuff, so this location will do for now. -- 2010-02-21 CrT


#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "log-if.h"

int log_if_fd = 0;	// Zero value means no logging. (We'd never log to stdin anyhow! :-)

#define MAX_BUF 4096

// If log_if_fd is nonzero, fprintf given
// message to it, preceded by a seconds.microseconds timestamp.
// A typical line looks like
//
//    1266769503.421967:  foo.c:  The 23 zots are barred.
///
void   log_if   (const char * fmt, ...) {

    if (!log_if_fd) {

       return;

    } else {

        int  len;
        int  seconds;
        int  microseconds;

        extern int   _lib7_time_gettimeofday   (int* microseconds);	// From		src/c/lib/time/timeofday.c

	char buf[ MAX_BUF ];

	va_list va;

	// Start by writing the timestamp into buf[].
	//
	// We match the timestamp formats in make_logstring in
        // 
        //     src/lib/src/lib/thread-kit/src/lib/logger.pkg
	// and src/lib/std/src/io/file-g.pkg
	//
	// Making the gettimeofday() system call here
	// is a little bit risky in that the system
        // call might change the behavior being debugged,
        // but I think the tracelog timestamps are valuable
        // enough to justify the risk:
        //
	seconds = _lib7_time_gettimeofday (&microseconds);
	sprintf(buf,"%8d  %10d.%06d:                                        ", getpid(), seconds, microseconds);

	// Now write the message proper into buf[],
        // right after the timestamp:
	//
        len = strlen( buf );

	va_start(va, fmt);
	vsnprintf(buf+len, MAX_BUF-len, fmt, va); 
	va_end(va);


	// Finish up by writing buf[]
        // contents to log_if_fd.
	//
	// write() is a low-level unbuffered
	// system call, so we do not need to
	// do a flush( log_if_fd ) -- there
	// is no such call at this level.
	//
	// Note that usually we need strlen(buf)+1
	// when dealing with null-terminated strings
	// but here we do not want to write the final
	// null, so strlen(buf) is in fact correct:
	//
	write( log_if_fd, buf, strlen(buf) );
    }
}
