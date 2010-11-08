/* heap-input.h
 *
 */


/*
###                 "The Navy is very old and very wise."
###
###                               -- Rudyard Kipling,
###                                  The Fringes of the Fleet
 */


#ifndef _HEAP_INPUT_
#define _HEAP_INPUT_

#include <stdio.h>

#ifndef _ADDR_HASH_
#include "addr-hash.h"
#endif

typedef struct {	    /* An input source for reading heap data.  We need */
			    /* this because the blaster may need to read from a */
			    /* stream that has already had characters read from it. */
    bool_t	needsSwap;	/* true, if the input bytes need to be swapped */
    FILE	*file;		/* the file descriptor to read from, once the */
				/* buffered characters are exhausted */
    Byte_t	*base;		/* the start of the bufferec characters */
    Byte_t	*buf;		/* the current position in the buffer */
    long	nbytes;
} inbuf_t;


/** Big-chunk relocation info **/

typedef struct {	/* big-chunk relocation info */
    Addr_t	    oldAddr;
    bigchunk_desc_t   *newChunk;
} bo_reloc_t;

typedef struct {	/* big-chunk region relocation info */
    Addr_t	    firstPage;	/* the address of the first page of the region */
    int		    nPages;	/* the number of pages in the region */
    bo_reloc_t	    **chunkMap;   /* the map from pages to big-chunk relocation */
				/* info. */
} bo_region_reloc_t;

/* Big-chunk region hash table interface */
#define LookupBORegion(table, bibopIndex)	\
	((bo_region_reloc_t *)AddrTableLookup(table, BIBOP_INDEX_TO_ADDR(bibopIndex)))

/* Utility routines */
extern lib7_val_t *HeapIO_ReadExterns (inbuf_t *bp);
extern status_t HeapIO_Seek (inbuf_t *bp, long offset);
extern status_t HeapIO_ReadBlock (inbuf_t *bp, void *blk, long len);

#endif /* !_HEAP_INPUT_ */


/* COPYRIGHT (c) 1993 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */

