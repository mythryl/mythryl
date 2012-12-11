// generate-win32-signal-table-c.c
//
// Generate the "win32-sigtable.c" file.

#include "../mythryl-config.h"

#include <signal.h>
#include <stdio.h>
#include "header-file-autogeneration-stuff.h"
#include "win32-sigtab.h"

#ifndef DST_FILE
#define DST_FILE "win32-sigtable.c"
#endif

main ()
{
    char*       filename      =  DST_FILE;
    char*       unique_string =  NULL;
    char*       progname      =  "src/c/config/generate-win32-signal-table-c.c";

    FILE* f;
    int   i;

    f = start_generating_header_file( filename, unique_string, progname );
            
    fprintf (f, "\n");

    fprintf (f, "static System_Constant signal_sysconsts_table_guts__local[NUM_SIGS] = {\n");
    for (i = 0; i < NUM_SIGS; i++) {
      fprintf(f, "\t{ %d, \"%s\" },\n", win32SigTab[i].n, win32SigTab[i].sname);
    }
    fprintf (f, "};\n");

    fprintf (f, "static Sysconsts signal_sysconsts_table__local = {\n");
    fprintf (f, "    /* constants_count */ NUM_SIGS,\n");
    fprintf (f, "    /* consts */    signal_sysconsts_table_guts__local\n");
    fprintf (f, "};\n");

    finish_generating_header_file( f, unique_string );

    exit (0);

}

// COPYRIGHT (c) 1996 Bell Laboratories, Lucent Technologies
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2012,
// released under Gnu Public Licence version 3.


