// mem-writer.c
//
// An implementation of the abstract writers
// on top of memory regions.


/*
###       "Without C we have only Obol, Pasal and BASI."
###                          -- Michael Feldman
*/

#include "../config.h"

#include "runtime-base.h"
#include "writer.h"

#ifndef BUFSIZ
#define BUFSIZ 4096
#endif

typedef struct buffer {
    Unt8	*base;
    Unt8	*next;
    Unt8	*top;
} wr_buffer_t;

static void Put   (Writer *wr, Val_Sized_Unt w);
static void Write (Writer *wr, const void *data, Punt nbytes);
static void Flush (Writer *wr);
static long Tell  (Writer *wr);
static void Seek  (Writer *wr, long offset);
static void Free  (Writer *wr);

#define BufOf(wr)	((wr_buffer_t *)((wr)->data))

Writer*   WR_OpenMem   (Unt8* data,  Punt len)   {
    //    ==========
    // 
    // Open a ram segment for writing, and make a writer for it.

    wr_buffer_t*	bp;
    Writer*	wr;

    bp = MALLOC_CHUNK(wr_buffer_t);
    bp->base	= data;
    bp->next	= data;
    bp->top	= (Unt8 *)(((Punt)data) + len);

    wr = MALLOC_CHUNK(Writer);
    wr->seen_error	= FALSE;
    wr->data	= (void *)bp;
    wr->put_word	= Put;
    wr->write	= Write;
    wr->flush	= Flush;
    wr->tell	= Tell;
    wr->seek	= Seek;
    wr->free	= Free;

    return wr;
}							// fun WR_OpenMem


static void   Put   (Writer* wr,  Val_Sized_Unt w)   {
    //        ===
    // 
    wr_buffer_t*  bp =   BufOf( wr );

    ASSERT(bp->next+WORD_BYTESIZE <= bp->top);

    *((Val_Sized_Unt *)(bp->next)) = w;

    bp->next += WORD_BYTESIZE;
}



static void   Write   (Writer* wr,  const void* data,  Punt nbytes)   {
    //        =====
    //
    wr_buffer_t*  bp =   BufOf( wr );

    if (wr->seen_error)   return;

    ASSERT(bp->next+nbytes <= bp->top);

    memcpy (bp->next, data, nbytes);

    bp->next += nbytes;
}



static void   Flush   (Writer* wr)   {
    //        =====
    //
    wr_buffer_t*  bp =   BufOf(wr);

    ASSERT(bp->next <= bp->top);
}



static long   Tell   (Writer* wr)   {
    //        ====
    //
    die ("Tell not supported on memory writers");
}



static void   Seek   (Writer* wr,  long offset)   {
    //        ====
    //
    die ("Tell not supported on memory writers");
}



static void   Free   (Writer* wr)   {
    //        ====
    //
    wr_buffer_t* bp = BufOf(wr);

    ASSERT( bp->next == bp->top );

    FREE( BufOf(wr) );
    FREE( wr );
}



// COPYRIGHT (c) 1995 AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

