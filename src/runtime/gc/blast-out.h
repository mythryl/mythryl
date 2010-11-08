/* blast-out.h
 *
 */

#ifndef _BLAST_OUT_
#define _BLAST_OUT_

#ifndef _ADDR_HASH_
#include "addr-hash.h"
#endif

#ifndef _C_GLOBALS_TABLE_
#include "c-globals-table.h"
#endif

#ifndef _WRITER_
#include "writer.h"
#endif

/* the table of referenced code chunks, and embedded literals */

typedef enum {
    EMB_STRING,		/* embedded string */
    EMB_REALD,		/* embedded real */
    UNUSED_CODE,	/* code chunk with only embedded references */
    USED_CODE		/* code chunk with code references */
} embchunk_kind_t;

typedef struct embchunk_info {    /* info about an embedded chunk */
    embchunk_kind_t	kind;
    struct embchunk_info  *codeChunk;	/* points to entry for the code */
					/* chunk that this literal is */
					/* embedded in. */
    lib7_val_t		relAddr;	/* the relocated address of the literal */
					/* in the blasted heap image. */
} embchunk_info_t;

/* find an embedded chunk */
#define FindEmbChunk(table, addr)	\
	((embchunk_info_t *)AddrTableLookup((table), (Addr_t)(addr)))


typedef struct {		/* the result of blasting out an chunk */
    bool_t	error;		    /* true, if there was an error during the */
				    /* blast GC (e.g., unrecognized external chunk) */
    bool_t	needsRepair;	    /* true, if the heap needs repair; otherwise */
				    /* the collection must be completed. */
    int		maxGen;		    /* the oldest generation included in the blast. */
    export_table_t *exportTable;	    /* the table of external chunks */
    addr_table_t	*embchunkTable;	    /* the table of embedded chunks */
} blast_res_t;

extern blast_res_t BlastGC (lib7_state_t *lib7_state, lib7_val_t *root, int gen);
Addr_t BlastGC_AssignLitAddresses (blast_res_t *res, int id, Addr_t offset);
void BlastGC_BlastLits (writer_t *wr);
extern void BlastGC_FinishUp (lib7_state_t *lib7_state, blast_res_t *res);

#endif  /* _BLAST_OUT_ */


/* COPYRIGHT (c) 1993 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */

