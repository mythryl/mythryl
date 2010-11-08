/* mem-writer.c
 *
 * An implementation of the abstract writers on top of memory regions.
 */

/* ###       "Without C we have only Obol, Pasal and BASI."
   ###                          -- Michael Feldman
 */

#include "../config.h"

#include "runtime-base.h"
#include "writer.h"

#ifndef BUFSIZ
#define BUFSIZ 4096
#endif

typedef struct buffer {
    Byte_t	*base;
    Byte_t	*next;
    Byte_t	*top;
} wr_buffer_t;

static void Put (writer_t *wr, Word_t w);
static void Write (writer_t *wr, const void *data, Addr_t nbytes);
static void Flush (writer_t *wr);
static long Tell (writer_t *wr);
static void Seek (writer_t *wr, long offset);
static void Free (writer_t *wr);

#define BufOf(wr)	((wr_buffer_t *)((wr)->data))

/* WR_OpenMem:
 *
 * Open a file for writing, and make a writer for it.
 */
writer_t *WR_OpenMem (Byte_t *data, Addr_t len)
{
    wr_buffer_t	*bp;
    writer_t	*wr;

    bp = NEW_CHUNK(wr_buffer_t);
    bp->base	= data;
    bp->next	= data;
    bp->top	= (Byte_t *)(((Addr_t)data) + len);

    wr = NEW_CHUNK(writer_t);
    wr->seen_error	= FALSE;
    wr->data	= (void *)bp;
    wr->putWord	= Put;
    wr->write	= Write;
    wr->flush	= Flush;
    wr->tell	= Tell;
    wr->seek	= Seek;
    wr->free	= Free;

    return wr;

} /* end of WR_OpenMem */

/* Put:
 */
static void Put (writer_t *wr, Word_t w)
{
    wr_buffer_t	*bp = BufOf(wr);

    ASSERT(bp->next+WORD_SZB <= bp->top);

    *((Word_t *)(bp->next)) = w;
    bp->next += WORD_SZB;

} /* end of Put */

/* Write:
 */
static void Write (writer_t *wr, const void *data, Addr_t nbytes)
{
    wr_buffer_t	*bp = BufOf(wr);

    if (wr->seen_error)
	return;

    ASSERT(bp->next+nbytes <= bp->top);

    memcpy (bp->next, data, nbytes);
    bp->next += nbytes;

} /* end of Write */

/* Flush:
 */
static void Flush (writer_t *wr)
{
    wr_buffer_t	*bp = BufOf(wr);

    ASSERT(bp->next <= bp->top);

} /* end of Flush */

/* Tell:
 */
static long Tell (writer_t *wr)
{
    Die ("Tell not supported on memory writers");

} /* end of Tell */

/* Seek:
 */
static void Seek (writer_t *wr, long offset)
{
    Die ("Tell not supported on memory writers");

} /* end of Seek */

/* Free:
 */
static void Free (writer_t *wr)
{
    wr_buffer_t	*bp = BufOf(wr);

    ASSERT(bp->next == bp->top);

    FREE (BufOf(wr));
    FREE (wr);

} /* end of Free */



/* COPYRIGHT (c) 1995 AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */

