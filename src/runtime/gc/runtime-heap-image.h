/* runtime-heap-image.h
 *
 * The definitions and typedefs that describe the layout of a Lib7
 * heap image in a file.  This can be either an exported heap, or
 * a blasted chunk.
 *
 * These files have the following basic layout:
 *
 *  Image header
 *  Heap/Blast header
 *  External reference table
 *  Image
 *
 * where the format of Image depends on the kind (heap vs. blast).
 */

#ifndef _LIB7_IMAGE_
#define _LIB7_IMAGE_

#ifndef _LIB7_SIZES_
#include "runtime-sizes.h"
#endif

#ifndef _LIB7_STATE_
#include "runtime-state.h"
#endif

#ifndef _HEAP_
#include "heap.h"
#endif

/* tag to identify image byte order */
#define ORDER		0x00112233

/* heap image version identifier (date in mmddyyyy form) */
#define IMAGE_MAGIC	0x09082004

/* blasted heap image version identifier (date in 00mmddyy form) */
#define BLAST_MAGIC	0x00070995

/* the kind of heap image */
#define EXPORT_HEAP_IMAGE	1
#define EXPORT_FN_IMAGE		2
#define BLAST_IMAGE		3
#define BLAST_UNBOXED		4	/* a blasted unboxed value */

#define SHEBANG_SIZE 256
typedef struct {		/* The magic number, and other version info */
    char            shebang[ SHEBANG_SIZE ];	/*  */
    Unsigned32_t    byteOrder;	/* ORDER tag */
    Unsigned32_t    magic;	/* magic number */
    Unsigned32_t    kind;	/* EXPORT_HEAP_IMAGE, etc. */
    char	    arch[12];	/* the exporting machine's architecture */
    char	    opsys[12];	/* the exporting machine's operating system */
} lib7_image_hdr_t;


typedef struct {		/* The header for a heap image */
    int		numVProcs;	/* The number of virtual processors */
    int		numGens;	/* The number of heap generations */
    int		numArenas;	/* The number of small-chunk arenas (one per kind) */
    int		numBOKinds;	/* The number of big-chunk kinds */
    int		numBORegions;	/* The number of big-chunk regions in the */
				/* exporting address space. */
    int		cacheGen;	/* The oldest cached generation */    
    Addr_t	allocSzB;	/* The size of the allocation arena */
				/* heap chunks that are referred to by the runtime */
    lib7_val_t	pervasiveStruct;/* the contents of PervasiveStruct */
    lib7_val_t	runtimeCompileUnit;/* The run-time system compilation unit root */
    lib7_val_t	mathVec;	/* The Math package root (if defined) */
} lib7_heap_hdr_t;

typedef struct {	    /* The header for a blasted chunk image */
    Unsigned32_t    numArenas;	/* The number of small-chunk arenas (one per kind) */
    Unsigned32_t    numBOKinds;	/* The number of big-chunk kinds */
    Unsigned32_t    numBORegions;/* The number of big-chunk regions in the */
				/* exporting address space. */
    bool_t	    hasCode;	/* true, if the blasted chunk contains code */
    lib7_val_t	    rootChunk;	/* The root chunk */
} lib7_blast_hdr_t;

typedef struct {	    /* The header for the extern table */
    int		numExterns;	/* The number of external symbols */
    int		externSzB;	/* The size (in bytes) of the string table area. */
} extern_table_hdr_t;


typedef struct {	    /* The image of an Lib7 virtual processor.  The live */
			    /* registers are those specified by RET_MASK, plus */
			    /* the current_thread, exception_fate and pc. */
    lib7_val_t	sigHandler;	/* the contents of Lib7SignalHandler */
    lib7_val_t	stdArg;
    lib7_val_t	stdCont;
    lib7_val_t	stdClos;
    lib7_val_t	pc;
    lib7_val_t	exception_fate;
    lib7_val_t	current_thread;
    lib7_val_t	calleeSave[CALLEESAVE];
} lib7_vproc_image_t;


/* The heap header consists of numGens generation descriptions, each of which
 * consists of (numArenas+numBOKinds) heap_arena_hdr_t records.  After the
 * generation descriptors, there are numBORegions bo_region_info_t records,
 * which are followed by the page aligned heap image follows the heap header.
 */

typedef struct {	    /* An arena header.  This is used for both the regular */
			    /* arenas and the big-chunk arena of a generation. */
    int		gen;		/* the generation of this arena */
    int		chunkKind;	/* the kind of chunks in this arena */
    Unsigned32_t offset;	/* the file position at which this arena starts. */
    union {			/* additional info */
	struct {		    /* info for regular arenas */
	    Addr_t	baseAddr;	/* the base address of this arena in the */
					/* exporting address space. */
	    Addr_t	sizeB;		/* the size of the live data in this arena */
	    Addr_t	roundedSzB;	/* the padded size of this arena in the */
					/* image file */
	}	    o;
	struct {		    /* info for the big-chunk arena */
	    int		numBigChunks;	/* the number of big-chunks in this */
					/* generation. */
	    int		numBOPages;	/* the number of big-chunk pages required. */
	}	    bo;
    }		info;
} heap_arena_hdr_t;

typedef struct {		/* a descriptor of a big-chunk region in the */
				/* exporting address space */
    Addr_t	baseAddr;	/* the base address of this big-chunk region in */
				/* the exporting address space.  Note that this */
				/* is the address of the header, not of the */
				/* first page. */
    Addr_t	firstPage;	/* the address of the first page of the region in */
				/* the exporting address space. */
    Addr_t	sizeB;		/* the total size of this big-chunk region */
				/* (including the header). */
} bo_region_info_t;

typedef struct {		/* a header for a big-chunk */
    int		gen;		/* the generation of this big-chunk */
    int		chunkKind;	/* the ilk of this big-chunk */
    Addr_t	baseAddr;	/* the base address of this big-chunk in the */
				/* exporting address space */
    Addr_t	sizeB;		/* the size of this big-chunk */
} bigchunk_hdr_t;


/** external references **/
#define isEXTERNTAG(w)		(isDESC(w) && (GET_TAG(w) == DTAG_extern))
#define EXTERNID(w)		GET_LEN(w)

/** Pointer tagging operations **/
#define HIO_ID_BITS		8
#define HIO_ADDR_BITS		(BITS_PER_WORD-HIO_ID_BITS)
#define HIO_ADDR_MASK		((1 << HIO_ADDR_BITS) - 1)

#define HIO_TAG_PTR(id,offset)	PTR_CtoLib7(((id)<<HIO_ADDR_BITS)|(Addr_t)(offset))
#define HIO_GET_ID(p)		(PTR_LIB7toADDR(p)>>HIO_ADDR_BITS)
#define HIO_GET_OFFSET(p)	(PTR_LIB7toADDR(p) & HIO_ADDR_MASK)

#endif /* !_LIB7_IMAGE_ */




/* COPYRIGHT (c) 1992 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */

