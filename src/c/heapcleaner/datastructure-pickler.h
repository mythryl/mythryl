// datastructure-pickler.h

#ifndef DATASTRUCTURE_PICKLER_H
#define DATASTRUCTURE_PICKLER_H

#include "address-hashtable.h"
#include "mythryl-callable-cfun-hashtable.h"

#ifndef _WRITER_
#include "writer.h"
#endif

// The table of referenced code chunks
// and embedded literals.
//
typedef enum {
    //
    EMBEDDED_STRING,		// Embedded string.
    EMBEDDED_FLOAT64,		// Embedded float.
    //
    UNUSED_CODE,		// Code chunk with only embedded references.
    USED_CODE			// Code chunk with code references.
    //
} Embedded_Chunk_Kind;

typedef struct embchunk_info {				// Info about an embedded chunk.
    //
    Embedded_Chunk_Kind	  kind;
    struct embchunk_info* containing_codechunk;		// Entrypoint for the code chunk in which this literal is embedded.
    Val		          relocated_address;		// The relocated address of the literal in the pickled heap image.
    //
} Embedded_Chunk_Info;


// Find an embedded chunk:
//
#define FIND_EMBEDDED_CHUNK( table, addr )	\
	((Embedded_Chunk_Info*) addresstable_look_up((table), (Vunt)(addr)))


// Pickler_Result:   The result of pickling a datastructure.
//
typedef struct {
    //
    Bool error;						// TRUE iff there was an error during the pickler's clean (e.g., unrecognized external chunk)
    Bool heap_needs_repair;				// TRUE iff the heap needs repair; otherwise the cleaning must be completed.
    int  oldest_agegroup_included_in_pickle;		// The oldest agegroup included in the datastructure pickle.
    //
    Heapfile_Cfun_Table*  cfun_table;			// The table of mythryl-callable C functions referenced in heap.
    Addresstable*         embedded_chunk_table;		// The table of embedded chunks.
    //
} Pickler_Result;

extern Pickler_Result  pickler__clean_heap			(Task* task,  Val* root,  int gen);
Vunt                   pickler__relocate_embedded_literals	(Pickler_Result* result,  int id,  Vunt offset);
void                   pickler__pickle_embedded_literals	(Writer* wr);
extern void            pickler__wrap_up				(Task* task,  Pickler_Result* result);

#endif  // DATASTRUCTURE_PICKLER_H


// COPYRIGHT (c) 1993 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2014,
// released per terms of SMLNJ-COPYRIGHT.


