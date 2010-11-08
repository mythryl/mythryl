/* runtime-heap.c
 *
 * Code to allocate and manipulate Lib7 heap blocks.
 *
 * MP Note: when invoking the garbage collector, we add the
 * requested size to reqSizeB, so that multiple processors
 * can request space at the same time.
 */

/*
###             Ballade to an Artificial Satellite
###
###		One inland summer I walked through rye
###		A wind at my heels that smelled of grain
###		And harried white clouds through a whistling sky
###		Where the great sun stalked and shook his mane
###		And roared so brightly across the plain
###		It gleamed and shimmered like alien sands
###		Ten years old I saw down a lane
###		The thunderous light on wonderstrands.
###
###		In ages before the world ran dry
###		What might the mapless not contain?
###		Atlantis gleamed like a dream to die
###		Avalon lay under faerie reign
###		Cibola guarded a golden plain
###		Tir nan Og was fairlocked Fand's
###		Sober men saw from a gull's road wain
###		The thunderous light on wonderstrands
###
###		Such clanging countries in cloud lands lie
###		But men grew weary and men grew sane
###		And they grew grown and so did I
###		And knew Tartessus was only Spain
###		No galleons call at Taprobane --
###		Ceylon with English -- no queenly hand
###		Wears gold from Punt nor sees the Dane
###		The thunderous light on wonderstrands.
###
###		Ahoy, Prince Andros, horizon's bane!
###		They always wait, the elven lands
###		An evening planet sheds again
###		The thunderous light on wonderstrands.
###
###                       -- Poul Anderson
 */


#include "../config.h"

#include "runtime-base.h"
#include "heap.h"
#include "runtime-heap.h"
#include "runtime-limits.h"
#include "runtime-mp.h"
#include <string.h>

/* A macro to check for necessary GC; on MP systems, this needs to be
 * a loop, since other processors may steal the memory before the
 * checking processor can use it.
 */
#ifdef MP_SUPPORT
#define IFGC(ap, szb)	\
	while ((! isACTIVE(ap)) || (AVAIL_SPACE(ap) <= (szb)))
#else
#define IFGC(ap, szb)	\
	if ((! isACTIVE(ap)) || (AVAIL_SPACE(ap) <= (szb)))
#endif

#ifdef COLLECT_STATS
#define COUNT_ALLOC(lib7_state, nbytes)	{	\
	heap_t		*__h = lib7_state->lib7_heap;	\
	CNTR_INCR(&(__h->numAlloc), (nbytes));	\
    }
#else
#define COUNT_ALLOC(lib7_state, nbytes)	/* null */
#endif


/* LIB7_CString:
 *
 * Allocate a Lib7 string using a C string as an initializer.
 * We assume that the string is small and can be allocated in
 * the allocation arena (generation zero).        XXX BUGGO FIXME
 */
lib7_val_t

LIB7_CString (lib7_state_t *lib7_state, const char *v)
{
    int len = ((v == NULL) ? 0 : strlen(v));

    if (len == 0) {
	return LIB7_string0;
    } else {

	int	    n = BYTES_TO_WORDS(len+1);  /* count "\0" too */

	lib7_val_t result = LIB7_AllocRaw32 (lib7_state, n);

	/* Zero the last word to allow fast (word) string comparisons,
	 * and to guarantee 0 termination:
	 */
	PTR_LIB7toC(Word_t, result)[n-1] = 0;
	strcpy (PTR_LIB7toC(char, result), v);

	SEQHDR_ALLOC (lib7_state, result, DESC_string, result, len);

	return result;
    }
}

/* LIB7_CStringList:
 *
 * Given a NULL terminated rw_vector of char *, build a list of Lib7 strings.
 */
lib7_val_t

LIB7_CStringList (lib7_state_t *lib7_state, char **strs)
{
/** NOTE: we should do something about possible GC!!! XXX BUGGO FIXME**/
    int		i;
    lib7_val_t	p, s;

    for (i = 0;  strs[i] != NULL;  i++)
	continue;

    p = LIST_nil;
    while (i-- > 0) {
	s = LIB7_CString(lib7_state, strs[i]);
	LIST_cons(lib7_state, p, s, p);
    }

    return p;
}

/* LIB7_AllocString:
 *
 * Allocate an uninitialized Lib7 string of length > 0.
 * This string is guaranteed to be padded to word size with 0 bytes,
 * and to be 0 terminated.
 */
lib7_val_t

LIB7_AllocString (lib7_state_t *lib7_state, int len)
{
    int		nwords = BYTES_TO_WORDS(len+1);

    ASSERT(len > 0);

    {   lib7_val_t result = LIB7_AllocRaw32 (lib7_state, nwords);

	/* zero the last word to allow fast (word) string comparisons,
	 * and to guarantee 0 termination:
	 */
	PTR_LIB7toC(Word_t, result)[nwords-1] = 0;

	SEQHDR_ALLOC (lib7_state, result/*=*/, DESC_string, result, len);

	return result;
    }
}

/* LIB7_AllocRaw32:
 *
 * Allocate an uninitialized chunk of raw32 data.
 */
lib7_val_t

LIB7_AllocRaw32 (lib7_state_t *lib7_state, int nwords)
{
    lib7_val_t	desc = MAKE_DESC(nwords, DTAG_raw32);
    lib7_val_t	result;
    Word_t	szb;

    ASSERT(nwords > 0);

    if (nwords <= SMALL_CHUNK_SZW) {

	LIB7_AllocWrite (lib7_state, 0, desc);
	result = LIB7_Alloc (lib7_state, nwords);

    } else {

	arena_t	*ap = lib7_state->lib7_heap->gen[0]->arena[STRING_INDEX];

	szb = WORD_SZB*(nwords + 1);
	BEGIN_CRITICAL_SECT(MP_GCGenLock)
	    IFGC (ap, szb+lib7_state->lib7_heap->allocSzB) {

	        /* We need to do a garbage collection: */
		ap->reqSizeB += szb;
		RELEASE_LOCK(MP_GCGenLock);
		    collect_garbage (lib7_state, 1);
		ACQUIRE_LOCK(MP_GCGenLock);
		ap->reqSizeB = 0;
	    }
	    *(ap->nextw++) = desc;
	    result = PTR_CtoLib7(ap->nextw);
	    ap->nextw += nwords;

	END_CRITICAL_SECT(MP_GCGenLock)

	COUNT_ALLOC(lib7_state, szb);

    }

    return result;
}

/* LIB7_ShrinkRaw32:
 *
 * Shrink a freshly allocated Raw32 vector.  This is used by the input routines
 * that must allocate space for input that may be excessive.
 */
void

LIB7_ShrinkRaw32 (lib7_state_t *lib7_state, lib7_val_t v, int nWords)
{
    int oldNWords = CHUNK_LEN(v);

    if (nWords == oldNWords)   return;

    ASSERT((nWords > 0) && (nWords < oldNWords));

    if (oldNWords > SMALL_CHUNK_SZW) {

	arena_t	*ap = lib7_state->lib7_heap->gen[0]->arena[STRING_INDEX];
	ASSERT(ap->nextw - oldNWords == PTR_LIB7toC(lib7_val_t, v)); 
	ap->nextw -= (oldNWords - nWords);

    } else {

	ASSERT(lib7_state->lib7_heap_cursor - oldNWords == PTR_LIB7toC(lib7_val_t, v)); 
	lib7_state->lib7_heap_cursor -= (oldNWords - nWords);
    }

    PTR_LIB7toC(lib7_val_t, v)[-1] = MAKE_DESC(nWords, DTAG_raw32);
}

/* LIB7_AllocRaw64:
 *
 * Allocate an uninitialized chunk of raw64 data.
 */
lib7_val_t

LIB7_AllocRaw64 (lib7_state_t *lib7_state, int nelems)
{
    int		nwords = DOUBLES_TO_WORDS(nelems);
    lib7_val_t	desc   = MAKE_DESC(nwords, DTAG_raw64);
    lib7_val_t	result;
    Word_t	szb;

    if (nwords <= SMALL_CHUNK_SZW) {

#ifdef ALIGN_REALDS
        /* Force REALD_SZB alignment: */
	lib7_state->lib7_heap_cursor = (lib7_val_t *)((Addr_t)(lib7_state->lib7_heap_cursor) | WORD_SZB);
#endif
	LIB7_AllocWrite (lib7_state, 0, desc);
	result = LIB7_Alloc (lib7_state, nwords);

    } else {

	arena_t	*ap = lib7_state->lib7_heap->gen[0]->arena[STRING_INDEX];
	szb = WORD_SZB*(nwords + 2);
	BEGIN_CRITICAL_SECT(MP_GCGenLock)
	  /* NOTE: we use nwords+2 to allow for the alignment padding */
	    IFGC (ap, szb+lib7_state->lib7_heap->allocSzB) {
	      /* we need to do a GC */
		ap->reqSizeB += szb;
		RELEASE_LOCK(MP_GCGenLock);
		    collect_garbage (lib7_state, 1);
		ACQUIRE_LOCK(MP_GCGenLock);
		ap->reqSizeB = 0;
	    }
#ifdef ALIGN_REALDS
	  /* Force REALD_SZB alignment (descriptor is off by one word) */
#  ifdef CHECK_HEAP
	    if (((Addr_t)ap->nextw & WORD_SZB) == 0) {
		*(ap->nextw) = (lib7_val_t)0;
		ap->nextw++;
	    }
#  else
	    ap->nextw = (lib7_val_t *)(((Addr_t)ap->nextw) | WORD_SZB);
#  endif
#endif
	    *(ap->nextw++) = desc;
	    result = PTR_CtoLib7(ap->nextw);
	    ap->nextw += nwords;
	END_CRITICAL_SECT(MP_GCGenLock)
	COUNT_ALLOC(lib7_state, szb-WORD_SZB);
    }

    return result;
}


lib7_val_t

LIB7_AllocCode   (   lib7_state_t*   lib7_state,
                     int              len
                 )
{
   /* Allocate an uninitialized Lib7 code chunk.
    *  Assume that len > 1.
    */

    heap_t	    *heap = lib7_state->lib7_heap;
    int		    allocGen = (heap->numGens < CODE_ALLOC_GEN)
			? heap->numGens
			: CODE_ALLOC_GEN;
    gen_t	    *gen = heap->gen[allocGen-1];
    bigchunk_desc_t   *dp;

    BEGIN_CRITICAL_SECT(MP_GCGenLock)
	dp = BO_Alloc (heap, allocGen, len);
	ASSERT(dp->gen == allocGen);
	dp->next = gen->bigChunks[CODE_INDEX];
	gen->bigChunks[CODE_INDEX] = dp;
	dp->chunkc = CODE_INDEX;
	COUNT_ALLOC(lib7_state, len);
    END_CRITICAL_SECT(MP_GCGenLock)

    return PTR_CtoLib7(dp->chunk);
}


/* LIB7_AllocBytearray:
 *
 * Allocate an uninitialized Lib7 bytearray.  Assume that len > 0.
 */
lib7_val_t

LIB7_AllocBytearray (lib7_state_t *lib7_state, int len)
{
    int		nwords = BYTES_TO_WORDS(len);
    lib7_val_t	res;

    res = LIB7_AllocRaw32 (lib7_state, nwords);

  /* zero the last word to allow fast (word) string comparisons, and to
   * guarantee 0 termination.
   */
    PTR_LIB7toC(Word_t, res)[nwords-1] = 0;

    SEQHDR_ALLOC (lib7_state, res, DESC_word8arr, res, len);

    return res;
}


/* LIB7_AllocRealdarray:
 *
 * Allocate an uninitialized Lib7 realarray.  Assume that len > 0.
 */
lib7_val_t

LIB7_AllocRealdarray (lib7_state_t *lib7_state, int len)
{
    lib7_val_t result =  LIB7_AllocRaw64 (lib7_state, len);

    SEQHDR_ALLOC (lib7_state, result, DESC_real64arr, result, len);

    return result;
}

/* LIB7_AllocArray:
 *
 * Allocate an Lib7 rw_vector using initVal as an initial value.
 * Assume that len > 0.
 */
lib7_val_t

LIB7_AllocArray (lib7_state_t *lib7_state, int len, lib7_val_t initVal)
{
    lib7_val_t	res, *p;
    lib7_val_t	desc = MAKE_DESC(len, DTAG_rw_vec_data);
    int		i;
    Word_t	szb;

    if (len > SMALL_CHUNK_SZW) {
	arena_t	*ap = lib7_state->lib7_heap->gen[0]->arena[ARRAY_INDEX];
	int	gcLevel = (isBOXED(initVal) ? 0 : -1);

	szb = WORD_SZB*(len + 1);
	BEGIN_CRITICAL_SECT(MP_GCGenLock)
#ifdef MP_SUPPORT
	  checkGC:;	/* the MP version jumps to here to recheck for GC */
#endif
	    if (! isACTIVE(ap)
	    || (AVAIL_SPACE(ap) <= szb+lib7_state->lib7_heap->allocSzB))
		gcLevel = 1;
	    if (gcLevel >= 0) {
	      /* we need to do a GC (and preserve initVal) */
		lib7_val_t	root = initVal;
		ap->reqSizeB += szb;
		RELEASE_LOCK(MP_GCGenLock);
		    collect_garbage_with_extra_roots (lib7_state, gcLevel, &root, NULL);
		    initVal = root;
		ACQUIRE_LOCK(MP_GCGenLock);
		ap->reqSizeB = 0;
#ifdef MP_SUPPORT
	      /* check again to insure that we have sufficient space */
		gcLevel = -1;
		goto checkGC;
#endif
	    }
	    ASSERT(ap->nextw == ap->sweep_nextw);
	    *(ap->nextw++) = desc;
	    res = PTR_CtoLib7(ap->nextw);
	    ap->nextw += len;
	    ap->sweep_nextw = ap->nextw;
	END_CRITICAL_SECT(MP_GCGenLock)
	COUNT_ALLOC(lib7_state, szb);
    }
    else {
	LIB7_AllocWrite (lib7_state, 0, desc);
	res = LIB7_Alloc (lib7_state, len);
    }

    for (p = PTR_LIB7toC(lib7_val_t, res), i = 0;  i < len; i++)
	*p++ = initVal;

    SEQHDR_ALLOC (lib7_state, res, DESC_polyarr, res, len);

    return res;
} /* end of LIB7_AllocArray. */


/* LIB7_AllocVector:
 *
 * Allocate an Lib7 vector, using the list initVal as an initializer.
 * Assume that len > 0.
 */
lib7_val_t

LIB7_AllocVector (lib7_state_t *lib7_state, int len, lib7_val_t initVal)
{
    lib7_val_t	desc = MAKE_DESC(len, DTAG_ro_vec_data);
    lib7_val_t	res, *p;

    if (len > SMALL_CHUNK_SZW) {
      /* Since we want to avoid pointers from the 1st generation record space
       * into the allocation space, we need to do a GC (and preserve initVal)
       */
	arena_t		*ap = lib7_state->lib7_heap->gen[0]->arena[RECORD_INDEX];
	lib7_val_t	root = initVal;
	int		gcLevel = 0;
	Word_t		szb;

	szb = WORD_SZB*(len + 1);
	BEGIN_CRITICAL_SECT(MP_GCGenLock)
	    if (! isACTIVE(ap)
	    || (AVAIL_SPACE(ap) <= szb+lib7_state->lib7_heap->allocSzB))
		gcLevel = 1;
#ifdef MP_SUPPORT
	  checkGC:;	/* the MP version jumps to here to redo the GC */
#endif
	    ap->reqSizeB += szb;
	    RELEASE_LOCK(MP_GCGenLock);
	        collect_garbage_with_extra_roots (lib7_state, gcLevel, &root, NULL);
	        initVal = root;
	    ACQUIRE_LOCK(MP_GCGenLock);
	    ap->reqSizeB = 0;
#ifdef MP_SUPPORT
	  /* check again to insure that we have sufficient space */
	    if (AVAIL_SPACE(ap) <= szb+lib7_state->lib7_heap->allocSzB)
		goto checkGC;
#endif
	    ASSERT(ap->nextw == ap->sweep_nextw);
	    *(ap->nextw++) = desc;
	    res = PTR_CtoLib7(ap->nextw);
	    ap->nextw += len;
	    ap->sweep_nextw = ap->nextw;
	END_CRITICAL_SECT(MP_GCGenLock)
	COUNT_ALLOC(lib7_state, szb);
    }
    else {
	LIB7_AllocWrite (lib7_state, 0, desc);
	res = LIB7_Alloc (lib7_state, len);
    }

    for (
	p = PTR_LIB7toC(lib7_val_t, res);
	initVal != LIST_nil;
	initVal = LIST_tl(initVal)
    )
	*p++ = LIST_hd(initVal);

    SEQHDR_ALLOC (lib7_state, res, DESC_polyvec, res, len);

    return res;

} /* end of LIB7_AllocVector. */


/* LIB7_SysConst:
 *
 * Find the system constant with the given id in table, and allocate a pair
 * to represent it.  If the constant is not present, then return the
 * pair (~1, "<UNKNOWN>").
 */
lib7_val_t LIB7_SysConst (lib7_state_t *lib7_state, sysconst_table_t *table, int id)
{
    lib7_val_t	name, res;
    int		i;

    for (i = 0;  i < table->numConsts;  i++) {
	if (table->consts[i].id == id) {
	    name = LIB7_CString (lib7_state, table->consts[i].name);
	    REC_ALLOC2 (lib7_state, res, INT_CtoLib7(id), name);
	    return res;
	}
    }
  /* here, we did not find the constant */
    name = LIB7_CString (lib7_state, "<UNKNOWN>");
    REC_ALLOC2 (lib7_state, res, INT_CtoLib7(-1), name);
    return res;

} /* end of LIB7_SysConst */


/* LIB7_SysConstList:
 *
 * Generate a list of system constants from the given table.
 */
lib7_val_t 

LIB7_SysConstList (lib7_state_t *lib7_state, sysconst_table_t *table)
{
    int		i;

    lib7_val_t	name;
    lib7_val_t	sysConst;
    lib7_val_t	list;

/** Should check for available heap space !!! XXX BUGGO FIXME **/

    for (list = LIST_nil, i = table->numConsts;  --i >= 0;  ) {
	name = LIB7_CString (lib7_state, table->consts[i].name);
	REC_ALLOC2 (lib7_state, sysConst, INT_CtoLib7(table->consts[i].id), name);
	LIST_cons(lib7_state, list, sysConst, list);
    }

    return list;
}


/* LIB7_AllocCData:
 *
 * Allocate a 64-bit aligned raw data chunk (to store abstract C data).
 */
lib7_val_t LIB7_AllocCData (lib7_state_t *lib7_state, int nbytes)
{
    lib7_val_t	chunk;

    chunk = LIB7_AllocRaw64 (lib7_state, (nbytes+7)>>2);

    return chunk;

} /* end of LIB7_AllocCData */



lib7_val_t

LIB7_CData (lib7_state_t *lib7_state, void *data, int nbytes)
{
    /* Allocate a 64-bit aligned raw data chunk and initialize it to the given C data:
     */

    lib7_val_t	chunk;

    if (nbytes == 0) {
	return LIB7_void;
    } else {
	chunk = LIB7_AllocRaw64 (lib7_state, (nbytes+7)>>2);
	memcpy (PTR_LIB7toC(void, chunk), data, nbytes);

	return chunk;
    }
}


/* COPYRIGHT (c) 1993 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */

