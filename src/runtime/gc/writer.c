/* writer.c
 *
 * An implementation of the abstract writers on top of ANSI C streams.
 */

#include "../config.h"

#include <stdio.h>
#include "runtime-base.h"
#include "writer.h"

#ifndef SEEK_SET
#  define SEEK_SET	0
#endif

static void Put (writer_t *wr, Word_t w);
static void Write (writer_t *wr, const void *data, Addr_t nbytes);
static void Flush (writer_t *wr);
static long Tell (writer_t *wr);
static void Seek (writer_t *wr, long offset);
static void Free (writer_t *wr);

#define FileOf(wr)	((FILE *)((wr)->data))

/* WR_OpenFile:
 *
 * Open a file for writing, and make a writer for it.
 */
writer_t *WR_OpenFile (FILE *f)
{
    writer_t	*wr;

    if (f == NULL)
	return NULL;

    wr = NEW_CHUNK(writer_t);
    wr->seen_error	= FALSE;
    wr->data	= (void *)f;
    wr->putWord	= Put;
    wr->write	= Write;
    wr->flush	= Flush;
    wr->tell	= Tell;
    wr->seek	= Seek;
    wr->free	= Free;

    return wr;

} /* end of WR_OpenFile */

/* Put:
 */
static void Put (writer_t *wr, Word_t w)
{
    FILE	*f = FileOf(wr);

    if (fwrite((void *)&w, WORD_SZB, 1, f) != 1) {
	wr->seen_error = TRUE;
    }

} /* end of Put */

/* Write:
 */
static void Write (writer_t *wr, const void *data, Addr_t nbytes)
{
    FILE	*f = FileOf(wr);

    if (fwrite(data, 1, nbytes, f) != nbytes) {
	wr->seen_error = TRUE;
    }

} /* end of Write */

/* Flush:
 */
static void Flush (writer_t *wr)
{
    fflush (FileOf(wr));

} /* end of Flush */

/* Tell:
 */
static long Tell (writer_t *wr)
{
    return ftell(FileOf(wr));

} /* end of Tell */

/* Seek:
 */
static void Seek (writer_t *wr, long offset)
{
    if (fseek(FileOf(wr), offset, SEEK_SET) != 0)
	wr->seen_error = TRUE;

} /* end of Seek */

/* Free:
 */
static void Free (writer_t *wr)
{
    fflush (FileOf(wr));
    FREE(wr);

} /* end of Free */


/* COPYRIGHT (c) 1995 AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */

