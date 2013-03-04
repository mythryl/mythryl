// passthrough.c, a trivial app which echoes stdin to stdout.

// History:
// 2012-07-04 CrT: No longer in use. I believe this was an experiment prior to writing   src/c/o/mythryl.c
// 2007-03-13 CrT: Created.

#include "../mythryl-config.h"

#include <stdio.h>		// For fopen() etc.
#include <stdlib.h>		// For exit().
#include <signal.h>		// For sigaction(), kill().

#if HAVE_SYS_SELECT_H
    #include <sys/select.h>	// For select().
#endif

#include <string.h>		// For strerror().
#include <errno.h>		// For errno().

#if HAVE_SYS_WAIT_H
    #include <sys/wait.h>	// For waitpid().
#endif

#if HAVE_SYS_TYPES_H
    #include <sys/types.h>	// For fstat(), getuid(), fork(), waitpid(), kill().
#endif

#if HAVE_SYS_STAT_H
    #include <sys/stat.h>	// For fstat().
#endif

#if HAVE_UNISTD_H
    #include <unistd.h>		// For fstat(), getuid(), fork(), dup(), pipe(), sleep()
#endif

#ifndef TRUE
    #define TRUE (1)
#endif

#ifndef FALSE
    #define FALSE (0)
#endif

char* our_name          = "(unknown)";



void   sleep_100ms   ( void ) {
    //
    // Sleep for 1/10 second:

    struct timeval tv;

    tv.tv_sec  = 0;
    tv.tv_usec = 100000;

    select( 0, NULL, NULL, NULL, &tv );
}





int   main   ( int argc, char** argv ) {

    char buf[ 512 ];

    for (;;)  {

        ssize_t   bytes_read
            =
            read(   STDIN_FILENO,   buf,   512   );

	if (bytes_read <= -1) {
	    fprintf( stderr,"passthrough: unable to read from fd %d: %s\n", STDIN_FILENO, strerror(errno) );
	    exit(1);
	}

	if (bytes_read == 0)  return 0;

        {   char* rest_of_buf
                =
                buf;

	    int bytes_left_to_write
                =
                bytes_read;

            // Loop until we've written all the bytes we read:
	    //
            while (bytes_left_to_write > 0) {
	        //
	        ssize_t   bytes_written
                    = 
                    write(   STDOUT_FILENO,   rest_of_buf,   bytes_left_to_write   );

                if (bytes_written <= -1) {
		    //
		    fprintf( stderr,"passthrough: unable to write to fd %d: %s\n", STDOUT_FILENO, strerror(errno) );
		    exit(1);
                }

                // A sane OS shouldn't put us in a busy-wait loop
                // by accepting zero bytes, but sane OSes are as
                // common as unicorns, so:
                //
                if (bytes_written == 0)   sleep_100ms ();

                rest_of_buf         += bytes_written;
                bytes_left_to_write -= bytes_written;
	    }         
        }
    }

    fprintf( stderr,"passthrough: DONE, exiting with status 13\n" );

    exit( 13 );
}

// Code by Jeff Prothero Copyright (c) 2010-2012,
// released per terms of SMLNJ-COPYRIGHT.
