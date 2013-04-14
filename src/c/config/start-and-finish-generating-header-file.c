// start-and-finish-generating-header-file.c
//
// Common code for generating header files.

#include "../mythryl-config.h"

#include <stdio.h>
#include <stdlib.h>
#include "header-file-autogeneration-stuff.h"

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 0
#endif

FILE*   start_generating_header_file   (char* fname, char* unique_string, char* progname) {

    // Open a generated file, and generate its header
    //
    //     #ifndef FOO_H
    //     #define FOO_H
    //
    // block.	

    FILE* fd;

    if ((fd = fopen(fname, "w")) == NULL) {
	fprintf (stderr, "start_generating_header_file(): FATAL: Unable to open file \"%s\"\n", fname);
	exit (1);
    }

    fprintf (fd, "// %s\n", fname);
    fprintf (fd, "//\n");
    fprintf (fd, "// This file was created by\n");
    fprintf (fd, "//     start_generating_header_file ()\n");
    fprintf (fd, "// from\n");
    fprintf (fd, "//     src/c/config/start-and-finish-generating-header-file.c\n");
    fprintf (fd, "// for\n");
    fprintf (fd, "//     %s\n", progname);
    fprintf (fd, "// Editing this file is probably a mistake. :-)\n");
    fprintf (fd, "//\n");
    fprintf (fd, "\n");
    if (unique_string) {
	fprintf (fd, "#ifndef %s\n", unique_string);
	fprintf (fd, "#define %s\n", unique_string);
	fprintf (fd, "\n");
    }

    return fd;
}


void   finish_generating_header_file   (FILE* fd, char* unique_string) {
    //
    // Generate the header file
    //
    //     #endif // FOO_H
    //
    // trailer block and close the generated file.
    //
    if (unique_string) {
	fprintf (fd, "\n");
	fprintf (fd, "#endif // !%s\n", unique_string);
    }

    fclose( fd );
}



// COPYRIGHT (c) 1994 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2013,
// released per terms of SMLNJ-COPYRIGHT.
