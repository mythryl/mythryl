// writer.c
//
// An implementation of the abstract writers on top of ANSI C streams.


#include "../mythryl-config.h"

#include <stdio.h>
#include "runtime-base.h"
#include "writer.h"

#ifndef SEEK_SET
#  define SEEK_SET	0
#endif

static void Put (Writer* wr, Vunt w);
static void Write (Writer* wr, const void *data, Punt nbytes);
static void Flush (Writer* wr);
static long Tell (Writer* wr);
static void Seek (Writer* wr, long offset);
static void Free (Writer* wr);

#define FileOf(wr)	((FILE*)((wr)->data))


Writer*   WR_OpenFile   (FILE* f)   {
    // 
    // Open a file for writing, and make a writer for it.
    ///
    Writer*	wr;

    if (f == NULL)   return NULL;

    wr = MALLOC_CHUNK(Writer);
    wr->seen_error	= FALSE;
    wr->data	= (void *)f;
    wr->put_word	= Put;
    wr->write	= Write;
    wr->flush	= Flush;
    wr->tell	= Tell;
    wr->seek	= Seek;
    wr->free	= Free;

    return wr;
}



static void   Put   (Writer* wr,  Vunt w)   {
    //
    FILE* f =  FileOf( wr );

    if (fwrite((void *)&w, WORD_BYTESIZE, 1, f) != 1) {
	wr->seen_error = TRUE;
    }
}



static void   Write   (Writer* wr,  const void* data,  Punt nbytes)   {
    //
    FILE* f =  FileOf( wr );

    if (fwrite(data, 1, nbytes, f) != nbytes) {
	wr->seen_error = TRUE;
    }
}



static void   Flush   (Writer* wr)   {
    //
    fflush (FileOf(wr));
}



static long   Tell   (Writer* wr)   {
    //
    return ftell(FileOf(wr));
}



static void   Seek   (Writer* wr,  long offset)   {
    //
    if (fseek(FileOf(wr), offset, SEEK_SET) != 0) {
	wr->seen_error = TRUE;
    }
}



static void   Free   (Writer* wr)   {
    //
    fflush (FileOf(wr));
    FREE(wr);
}


// COPYRIGHT (c) 1995 AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2012,
// released under Gnu Public Licence version 3.


