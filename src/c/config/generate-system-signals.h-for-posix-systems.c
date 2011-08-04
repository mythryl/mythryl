// generate-system-signals.h-for-posix-systems.c
//
// Generate the "system-signals.h" file for UNIX systems.

#include "../config.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include "header-file-autogeneration-stuff.h"
#include "generate-system-signals.h-for-posix-systems.h"

#ifndef DST_FILE
#define DST_FILE "system-signals.h"
#endif


int main (void)
{
    Runtime_System_Signal_Table* signal_db;
    int		    i;
    int		    numSigs;
    FILE	    *f;

    char*       filename      = DST_FILE;
    char*       unique_string = "SYSTEM_SIGNALS_H";
    char*       progname      = "src/c/config/generate-system-signals.h-for-posix-systems.c";

    signal_db = sort_runtime_system_signal_table ();

    f = start_generating_header_file( filename, unique_string, progname );		// start_generating_header_file	is from   src/c/config/start-and-finish-generating-header-file.c

    numSigs = signal_db->posix_signal_kinds + signal_db->runtime_generated_signal_kinds;

    fprintf (f, "#define NUM_SYSTEM_SIGS %2d\n", signal_db->posix_signal_kinds);
    fprintf (f, "#define MIN_SYSTEM_SIG  %2d /* %s */\n",
	signal_db->lowest_valid_posix_signal_number, signal_db->sigs[0]->sigName);
    fprintf (f, "#define MAX_SYSTEM_SIG  %2d /* %s */\n",
	signal_db->highest_valid_posix_signal_number, signal_db->sigs[signal_db->posix_signal_kinds-1]->sigName);
    fprintf (f, "#define NUM_SIGS        %2d\n", numSigs);
    fprintf (f, "#define MAX_POSIX_SIGNALS       %2d\n",
	signal_db->highest_valid_posix_signal_number + signal_db->runtime_generated_signal_kinds + 1);
    fprintf (f, "\n");
    for (i = signal_db->posix_signal_kinds;  i < numSigs;  i++) {
	fprintf(f, "#define %s %2d\n",
	    signal_db->sigs[i]->sigName, signal_db->sigs[i]->sig);
    }
    fprintf (f, "\n");

    fprintf (f, "#define IS_SYSTEM_SIG(S) ((S) <= MAX_SYSTEM_SIG)\n");

    finish_generating_header_file( f, unique_string );

    exit( 0 );
}

// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

