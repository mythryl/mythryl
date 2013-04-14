// header-file-autogeneration-stuff.h


#ifndef HEADER_FILE_AUTOGENERATION_STUFF_H
#define HEADER_FILE_AUTOGENERATION_STUFF_H

#include <stdio.h>

extern FILE*   start_generating_header_file   (char* fname, char* unique_string, char* progname);
extern void   finish_generating_header_file   (FILE* f,     char* unique_string );

#ifndef RUNTIME_BASE_H

    #define MALLOC(size)	malloc(size)				// Aliases for malloc/free, so that we can easily replace them.
    #define FREE(p)		free(p)

    #define MALLOC_CHUNK(type)	((type*)MALLOC(sizeof(type)))		// Allocate a new C ram-chunk of type 'type'.

    #define MALLOC_VEC(type,n)	((type*)MALLOC((n)*sizeof(type)))	// Allocate a new C array of type 'type' chunks.

#endif // RUNTIME_BASE_H

#endif // HEADER_FILE_AUTOGENERATION_STUFF_H



// COPYRIGHT (c) 1994 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2013,
// released per terms of SMLNJ-COPYRIGHT.

