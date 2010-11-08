/* heap-in-util.c
 *
 * Utility routines to import an Lib7 heap image.
 */

#include "../config.h"

#include "runtime-base.h"
#include "heap.h"
#include "runtime-values.h"
#include "runtime-heap-image.h"
#include "c-globals-table.h"
#include "heap-input.h"
#include <string.h>

#ifndef SEEK_SET
#  define SEEK_SET	0
#  define SEEK_END	2
#endif

/* local routines */
static status_t ReadBlock (FILE *file, void *blk, long len);


/*
###           "Every big computing disaster has
###            come from taking too many ideas
###            and putting them in one place."
###
###                          -- Gordon Bell
 */


/* HeapIO_ReadExterns:
 */
lib7_val_t *HeapIO_ReadExterns (inbuf_t *bp)
{
    extern_table_hdr_t	header;
    lib7_val_t		*externs;
    Byte_t		*buf, *cp;
    int			i;

  /* Read the header */
    HeapIO_ReadBlock(bp, &(header), sizeof(header));

    externs = NEW_VEC(lib7_val_t, header.numExterns);

  /* Read in the names of the exported symbols */
    buf = NEW_VEC(Byte_t, header.externSzB);
    HeapIO_ReadBlock (bp, buf, header.externSzB);

  /* map the names of the external symbols to addresses in the run-time system */
    for (cp = buf, i = 0;  i < header.numExterns;  i++) {
	if ((externs[i] = ImportCSymbol ((char *)cp)) == LIB7_void)
	    Die ("Run-time system does not provide \"%s\"", cp);
	cp += (strlen(cp) + 1);
    }
    FREE (buf);

    return externs;

} /* end of HeapIO_ReadExterns */


/* HeapIO_Seek:
 *
 * Adjust the next character position to the given position in the
 * input stream.
 */
status_t HeapIO_Seek (inbuf_t *bp, long offset)
{
    if (bp->file == NULL) {
      /* the stream is in-memory */
	Byte_t	*newPos = bp->base + offset;
	if (bp->buf + bp->nbytes <= newPos)
	    return FAILURE;
	else {
	    bp->nbytes -= (newPos - bp->buf);
	    bp->buf = newPos;
	    return SUCCESS;
	}
    }
    else {
      if (fseek (bp->file, offset, SEEK_SET) != 0)
	Die ("unable to seek on heap image\n");
      bp->nbytes = 0;		/* just in case? */
    }

} /* end of HeapIO_Seek */


/* HeapIO_ReadBlock:
 */
status_t HeapIO_ReadBlock (inbuf_t *bp, void *blk, long len)
{
    status_t	status = SUCCESS;

    if (bp->nbytes == 0) {
	if (bp->file != NULL)
	    status = ReadBlock (bp->file, blk, len);
	else {
	    Error ("missing data in memory blast chunk");
	    return FAILURE;
	}
    }
    else if (bp->nbytes >= len) {
	memcpy (blk, bp->buf, len);
	bp->nbytes -= len;
	bp->buf += len;
    }
    else {
	memcpy (blk, bp->buf, bp->nbytes);
	status = ReadBlock (bp->file, ((Byte_t *)blk) + bp->nbytes, len - bp->nbytes);
	bp->nbytes = 0;
    }

    if (bp->needsSwap) {
	Die ("byte-swapping not implemented yet");
    }

    return status;

} /* end of HeapIO_ReadBlock */

/* ReadBlock:
 */
static status_t ReadBlock (FILE *file, void *blk, long len)
{
    int		status;
    Byte_t	*bp = (Byte_t *)blk;

    while (len > 0) {
	status = fread (bp, 1, len, file);
	len -= status;
	bp += status;
	if ((status < len) && (ferror(file) || feof(file))) {
	    Error ("unable to read %d bytes from image\n", len);
	    return FAILURE;
	}
    }

    return SUCCESS;

} /* end of ReadBlock. */


/* COPYRIGHT (c) 1993 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */

