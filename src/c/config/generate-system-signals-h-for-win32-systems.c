// generate-system-signals-h-for-win32-systems.c
//
// Generate the "system-signals.h" file for Win32 systems.
// signals aren't currently implemented (since Win32 doesn't have signals)

#include "../mythryl-config.h"

#include <signal.h>
#include <stdio.h>
#include "header-file-autogeneration-stuff.h"
#include "win32-sigtab.h"

#ifndef DST_FILE
#define DST_FILE "system-signals.h"
#endif

int main () {
    //
    char*       filename      =  DST_FILE;
    char*       unique_string =  "SYSTEM_SIGNALS_H";
    char*       progname      =  "src/c/config/generate-system-signals-h-for-win32-systems.c";

    FILE*	f;
    int		numSigs = 1;
    int		i;

    f = start_generating_header_file( filename, unique_string, progname );
            

    fprintf (f, "#define NUM_SYSTEM_SIGS %2d\n", 0);
    fprintf (f, "#define MIN_SYSTEM_SIG  %2d /* %s */\n",
	     0, "none");
    fprintf (f, "#define MAX_SYSTEM_SIG  %2d /* %s */\n",
	     0, "none");
    fprintf (f, "#define NUM_SIGS        %2d\n", NUM_SIGS);
    fprintf (f, "#define MAX_SIG         %2d\n", NUM_SIGS);
    fprintf (f, "#define MAX_POSIX_SIGNALS       %2d\n", NUM_SIGS+1);
    fprintf (f, "\n");

    // The signals:
    //
    for (i = 0; i < NUM_SIGS; i++) {
        //
        fprintf(f, "#define %s %2d\n",  win32SigTab[i].lname, win32SigTab[i].n);
    }

    fprintf (f, "#define IS_SYSTEM_SIG(S) (0)\n");

    finish_generating_header_file( f, unique_string );

    exit (0);
}


// COPYRIGHT (c) 1996 by Bell Laboratories, Lucent Technologies
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

