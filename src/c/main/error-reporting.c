// error-reporting.c
//
// Run-time system error messages.


#include "../mythryl-config.h"

#include <stdio.h>
#include <stdarg.h>
#include "runtime-base.h"

extern FILE	*DebugF;	// Referenced only here and in   src/c/main/runtime-main.c
				// Defaults to stderr, may be set via   --runtime-debug=foo.log

void   say   (char *fmt, ...)   {
    // ===
    //
    // Print a message to the standard output.

    va_list	ap;

    va_start (ap, fmt);
    vfprintf (stdout, fmt, ap);
    va_end(ap);
    fflush (stdout);
}


void   debug_say   (char *format, ...)   {
    // =========
    //
    // Print a message to the debug output stream.

    va_list	ap;

    va_start (ap, format);
    vfprintf (DebugF, format, ap);
    va_end(ap);
    fflush (DebugF);
}


void   say_error   (char *fmt, ...)   {
    // =========
    //
    // Print an error message.

    va_list	ap;

    va_start (ap, fmt);
    fprintf (stderr, "%s: Nonfatal error:  ", mythryl_program_name__global);
    vfprintf (stderr, fmt, ap);
    va_end(ap);
}


void   die   (char *fmt, ...)   {
    // ===
    //
    // Print an error message and then exit.

    va_list	ap;

    va_start (ap, fmt);
    fprintf (stderr, "%s: Fatal error:  ", mythryl_program_name__global);
    vfprintf (stderr, fmt, ap);
    fprintf (stderr, "\n");
    va_end(ap);

    #if NEED_PTHREAD_SUPPORT
	// Release any platform-specific multicore-support
	// resources such as kernel locks or mmapped segments:
	//
	pth__shut_down ();				// pth__shut_down		defined in   src/c/pthread/pthread-on-posix-threads.c
							// pth__shut_down		defined in   src/c/pthread/pthread-on-sgi.c
    #endif						// pth__shut_down		defined in   src/c/pthread/pthread-on-solaris.c

    print_stats_and_exit( 1 );
}


#ifdef ASSERT_ON
    //
void   assert_fail   (const char* a,  const char* file,  int line)    {		// Used (optionally) in ASSERT macro in   src/c/h/runtime-base.h
	//
	//
	// Print an assertion failure message.

	fprintf (stderr, "%s: Fatal error:  Assertion failure (%s) at \"%s:%d\"\n", mythryl_program_name__global, a, file, line);

	#if NEED_PTHREAD_SUPPORT
	    pth__shut_down ();
	#endif

	print_stats_and_exit( 2 );
    }
#endif



// log_if implementation.
//
// See overview comments in
//
//     c/h/runtime-base.h
//


#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>


int log_if_fd = 0;	// Zero value means no logging. (We'd never log to stdin anyhow! :-)

#define MAX_BUF 4096

// If log_if_fd is nonzero, fprintf given
// message to it, preceded by a seconds.microseconds timestamp.
// A typical line looks like
//
//    time=1266769503.421967 pid=00000007 tid=00000000 name=none msg=foo.c:  The 23 zots are barred.
///
void   log_if   (const char * fmt, ...) {

    if (!log_if_fd) {
	//
        return;
	//
    } else {
	//
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

	// The intent here is
	//
	//   1) That doing unix 'sort' on a logfile will do the right thing:
	//      sort first by time, then by process id, then by thread id.
	//
	//   2) To facilitate egrep/perl processing, e.g. doing stuff like
	//            egrep 'pid=021456' logfile
	//
	// We fill in dummy tid= and (thread) name= values here to reduce
	// the need for special-case code when processing logfiles:
	//
	sprintf(buf,"time=%10d.%06d pid=%08d tid=00000000 name=%-16s msg=", seconds, microseconds, getpid(), "none");

	// Now write the message proper into buf[],
        // right after the timestamp:
	//
        len = strlen( buf );

	// Drop leading blanks:
	//
	while (*fmt == ' ') ++fmt;

	va_start(va, fmt);
	vsnprintf(buf+len, MAX_BUF-len, fmt, va); 
	va_end(va);

	// Append a newline to buffer:
	//
	strcpy( buf + strlen(buf), "\n" );

	// Finish up by writing buf[]
        // contents to log_if_fd.
	//
	// write() is a low-level unbuffered
	// system call, so we do not need to
	// do a flush( log_if_fd ) -- there
	// is no such call at this level.
	//
	// (We are using the low-level write() call
	// to guarantee that each write() of a line
	// is atomic.)
	//
	// Note that usually we need strlen(buf)+1
	// when dealing with null-terminated strings
	// but here we do not want to write the final
	// null, so strlen(buf) is in fact correct:
	//
	write( log_if_fd, buf, strlen(buf) );
    }
}


// COPYRIGHT (c) 1994 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.


