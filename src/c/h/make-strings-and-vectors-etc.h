// make-strings-and-vectors-etc.h
//
// Macros and routines for allocating heap chunks.
//
// See also:
//     src/c/heapcleaner/make-strings-and-vectors-etc.c


#ifndef RUNTIME_HEAP_H
#define RUNTIME_HEAP_H

#include "runtime-base.h"
#include "runtime-values.h"
#include "heap-tags.h"


// Extract info from chunks:
//
#define CHUNK_TAGWORD(CHUNK)		GET_TUPLE_SLOT_AS_VAL((CHUNK), -1)
#define CHUNK_LENGTH(CHUNK)		GET_LENGTH_IN_WORDS_FROM_TAGWORD(CHUNK_TAGWORD(CHUNK))
#define CHUNK_BTAG(CHUNK)		  GET_BTAG_FROM_TAGWORD(CHUNK_TAGWORD(CHUNK))		// Nowhere referenced.


// The size-in-bytes of a Mythryl record,
// including descriptor:
//
#define REC_BYTESIZE(n)	(((n)+1)*sizeof(Val))



// For background on using the following two fns see
// Note[1]: Heap record allocation at bottom of file:
//
inline void   set_slot_in_nascent_heapchunk   (Task* task, int i, Val v)	{
    //        =============================
    //
    task->heap_allocation_pointer[i] =  v;
}
//
inline Val    commit_nascent_heapchunk    (Task* task, int length_in_words)	{
    //        ========================
    //
    task->heap_allocation_pointer +=  length_in_words +1;					// Allocate agegroup0 heapspace for record.  ('+1' for tagword.)
    //
    return PTR_CAST( Val, task->heap_allocation_pointer - length_in_words );			// Return pointer to first work in record proper -- the word *after* the tagword.
}


inline Val   make_refcell   (Task* task, Val v)  {
    //       ============
    //
    Val* p = task->heap_allocation_pointer;
    //
    *p++ =  REFCELL_TAGWORD;      Val result = (Val) p;
    *p++ =  v;
    //
    task->heap_allocation_pointer = p;   
    //
    return result;
}
      

inline Val   make_one_slot_record   (Task* task, Val a) {
    //
    Val* p = task->heap_allocation_pointer;
    //
    *p++ =  MAKE_TAGWORD(1, PAIRS_AND_RECORDS_BTAG);      Val result = (Val) p;
    *p++ =  a;
    //
    task->heap_allocation_pointer = p;   
    //
    return result;
}

inline Val   make_two_slot_record   (Task* task, Val a, Val b) {
    //
    Val* p = task->heap_allocation_pointer;
    //
    *p++ =  PAIR_TAGWORD;      Val result = (Val) p;
    *p++ =  a;
    *p++ =  b;
    //
    task->heap_allocation_pointer = p;   
    //
    return result;
}

inline Val   make_three_slot_record   (Task* task, Val a, Val b, Val c) {
    //
    Val* p = task->heap_allocation_pointer;
    //
    *p++ =  MAKE_TAGWORD(3, PAIRS_AND_RECORDS_BTAG);      Val result = (Val) p;
    *p++ =  a;
    *p++ =  b;
    *p++ =  c;
    //
    task->heap_allocation_pointer = p;   
    //
    return result;
}

inline Val   make_four_slot_record   (Task* task,  Val a, Val b, Val c, Val d) {
    //
    Val* p = task->heap_allocation_pointer;
    //
    *p++ =  MAKE_TAGWORD(4, PAIRS_AND_RECORDS_BTAG);      Val result = (Val) p;
    *p++ =  a;
    *p++ =  b;
    *p++ =  c;
    *p++ =  d;
    //
    task->heap_allocation_pointer = p;   
    //
    return result;
}

inline Val   make_five_slot_record   (Task* task,  Val a, Val b, Val c, Val d, Val e) {
    //
    Val* p = task->heap_allocation_pointer;
    //
    *p++ =  MAKE_TAGWORD(5, PAIRS_AND_RECORDS_BTAG);      Val result = (Val) p;
    *p++ =  a;
    *p++ =  b;
    *p++ =  c;
    *p++ =  d;
    *p++ =  e;
    //
    task->heap_allocation_pointer = p;   
    //
    return result;
}

inline Val   make_six_slot_record   (Task* task,  Val a, Val b, Val c, Val d, Val e, Val f) {
    //
    Val* p = task->heap_allocation_pointer;
    //
    *p++ =  MAKE_TAGWORD(6, PAIRS_AND_RECORDS_BTAG);      Val result = (Val) p;
    *p++ =  a;
    *p++ =  b;
    *p++ =  c;
    *p++ =  d;
    *p++ =  e;
    *p++ =  f;
    //
    task->heap_allocation_pointer = p;   
    //
    return result;
}

inline Val   make_seven_slot_record   (Task* task,  Val a, Val b, Val c, Val d, Val e, Val f, Val g) {
    //
    Val* p = task->heap_allocation_pointer;
    //
    *p++ =  MAKE_TAGWORD(7, PAIRS_AND_RECORDS_BTAG);      Val result = (Val) p;
    *p++ =  a;
    *p++ =  b;
    *p++ =  c;
    *p++ =  d;
    *p++ =  e;
    *p++ =  f;
    *p++ =  g;
    //
    task->heap_allocation_pointer = p;   
    //
    return result;
}

inline Val   make_eight_slot_record   (Task* task,  Val a, Val b, Val c, Val d, Val e, Val f, Val g, Val h) {
    //
    Val* p = task->heap_allocation_pointer;
    //
    *p++ =  MAKE_TAGWORD(8, PAIRS_AND_RECORDS_BTAG);      Val result = (Val) p;
    *p++ =  a;
    *p++ =  b;
    *p++ =  c;
    *p++ =  d;
    *p++ =  e;
    *p++ =  f;
    *p++ =  g;
    *p++ =  h;
    //
    task->heap_allocation_pointer = p;   
    //
    return result;
}

inline Val   make_vector_header   (Task* task,  Val tagword, Val vectordata, int vectorlen) {
    //
    Val* p = task->heap_allocation_pointer;
    //
    *p++ =  tagword;      Val result = (Val) p;
    *p++ =  vectordata;
    *p++ =  TAGGED_INT_FROM_C_INT( vectorlen );
    //
    task->heap_allocation_pointer = p;   
    //
    return result;
}



#ifdef ALIGN_FLOAT64S									// The normal case for our current 32-bit implementation.

    inline Val   make_float64   (Task* task,  double d) {
	//
	Val* p =  task->heap_allocation_pointer;
	//
	p      =  (Val*)((Vunt)p | WORD_BYTESIZE);					// After this we are guaranteed that p is NOT  8-byte-aligned.
	//
	*p++   =  FLOAT64_TAGWORD;		      Val result = (Val) p;		// After this we are guaranteed that p IS     8-byte-aligned.

	*(double*)p =  d;								// Store the eight-byte-float eight-byte-aligned.

	p     +=  FLOAT64_SIZE_IN_WORDS;

	task->heap_allocation_pointer = p;   

	return result;
    }

#else

    inline Val   make_float64   (Task* task,  double d) {
	//
	Val* p =  task->heap_allocation_pointer;
	//
	*p++   =  FLOAT64_TAGWORD;		      Val result = (Val) p;

	*(double*)p =  d;								// Store the eight-byte-float eight-byte-aligned.

	p     +=  FLOAT64_SIZE_IN_WORDS;

	task->heap_allocation_pointer = p;   

	return result;
    }

#endif



#define MAKE_EXCEPTION(task, id, val, where) \
	make_three_slot_record(task, id, val, where)

// Boxed word values
//
#define WORD_LIB7toC(w)		(*PTR_CAST(Vunt*, w))

inline Val   make_one_word_unt   (Task* task, Vunt u) {
    //
    Val* p =  task->heap_allocation_pointer;
    //
    *p++   = MAKE_TAGWORD(1, FOUR_BYTE_ALIGNED_NONPOINTER_DATA_BTAG);		      Val result = (Val) p;
    *p++   = (Val) u;
    //
    task->heap_allocation_pointer = p;   
    //
    return  result;
}

inline Val   make_one_word_int   (Task* task, Vint i) {
    //
    Val* p =  task->heap_allocation_pointer;
    //
    *p++   = MAKE_TAGWORD(1, FOUR_BYTE_ALIGNED_NONPOINTER_DATA_BTAG);		      Val result = (Val) p;
    *p++   = (Val) i;
    //
    task->heap_allocation_pointer = p;   
    //
    return  result;
}


#define TUPLE_GETWORD(p, i)	(*GET_TUPLE_SLOT_AS_PTR(Vunt*, p, i))
#define INT1_LIB7toC(i)		(*PTR_CAST(Int1*, i))
#define TUPLE_GET_INT1(p, i)	(*GET_TUPLE_SLOT_AS_PTR(Int1*, p, i))


//////////////////
// Mythryl lists
//
#define LIST_HEAD(p)			GET_TUPLE_SLOT_AS_VAL(p, 0)
#define LIST_TAIL(p)			GET_TUPLE_SLOT_AS_VAL(p, 1)
#define LIST_CONS(task,a,b)		make_two_slot_record(task,a,b)
//
#define LIST_NIL			TAGGED_INT_FROM_C_INT(0)
#define LIST_IS_NULL(p)			((p) == LIST_NIL)


///////////////////////
// Mythryl references
//
#define DEREF(r)			GET_TUPLE_SLOT_AS_VAL(r, 0)
#define ASSIGN(r, x)			(PTR_CAST(Val*, r)[0] = (x))


//////////////////
// Mythryl options (Null_Or):
//
#define OPTION_NULL			TAGGED_INT_FROM_C_INT(0)
#define OPTION_THE(task, a)		make_one_slot_record(task, a)
#define OPTION_GET(r)			GET_TUPLE_SLOT_AS_VAL(r, 0)
    //
    // XXX BUGGO FIXME.  Let's find a way to make OPTION_THE and OPTION_GET into no-ops!
    //                   OPTION_NULL would need to become some otherwise unused heap value.


////////////////////
// External routines:
//
// NB: We use "make"  for fns which initialize the chunks' contents,
//     and "allocate" for fns which do not. "nonempty" is a reminder
//     that zero length is not ok.
//
extern Val  make_ascii_string_from_c_string__may_heapclean		(Task* task,  const char*, Roots*);	// make_ascii_string_from_c_string__may_heapclean		def in   src/c/heapcleaner/make-strings-and-vectors-etc.c
extern Val  make_ascii_strings_from_vector_of_c_strings__may_heapclean	(Task* task,  char**,      Roots*);	// make_ascii_strings_from_vector_of_c_strings__may_heapclean	def in   src/c/heapcleaner/make-strings-and-vectors-etc.c
extern Val  allocate_nonempty_vector_of_one_byte_unts__may_heapclean	(Task* task,  int len,	   Roots*);	// allocate_nonempty_vector_of_one_byte_unts__may_heapclean	def in   src/c/heapcleaner/make-strings-and-vectors-etc.c

extern Val  allocate_nonempty_ascii_string__may_heapclean		(Task* task,  int len,     Roots*);	// allocate_nonempty_ascii_string__may_heapclean		def in   src/c/heapcleaner/make-strings-and-vectors-etc.c
extern Val  allocate_headerless_ascii_string__may_heapclean	(Task* task,  int len,     Roots*);	// allocate_headerless_ascii_string__may_heapclean	def in   src/c/heapcleaner/make-strings-and-vectors-etc.c
		//
		// The 'headerless' version is just for special
		// internal use -- you usually want the other one.
//
extern Val  allocate_nonempty_code_chunk				(Task* task,  int len);			// allocate_nonempty_code_chunk					def in   src/c/heapcleaner/make-strings-and-vectors-etc.c
//
extern Val  allocate_headerless_rw_vector__may_heapclean		(Task* task,  int len, Bool,Roots*);	// allocate_headerless_rw_vector__may_heapclean			def in   src/c/heapcleaner/make-strings-and-vectors-etc.c
extern Val  allocate_headerless_ro_pointers_chunk__may_heapclean	(Task* task,  int len, int, Roots*);	// allocate_headerless_ro_pointers_chunk__may_heapclean		def in   src/c/heapcleaner/make-strings-and-vectors-etc.c
extern Val  make_nonempty_rw_vector__may_heapclean			(Task* task,  int len, Val, Roots*);	// make_nonempty_rw_vector__may_heapclean			def in   src/c/heapcleaner/make-strings-and-vectors-etc.c
extern Val  make_nonempty_ro_vector__may_heapclean			(Task* task,  int len, Val, Roots*);	// make_nonempty_ro_vector__may_heapclean			def in   src/c/heapcleaner/make-strings-and-vectors-etc.c
extern Val  allocate_nonempty_wordslots_vector__may_heapclean		(Task* task,  int len,      Roots*);	// allocate_nonempty_wordslots_vector__may_heapclean		def in   src/c/heapcleaner/make-strings-and-vectors-etc.c
extern void shrink_fresh_wordslots_vector				(Task* task,  Val v, int new_len);	// shrink_fresh_wordslots_vector				def in   src/c/heapcleaner/make-strings-and-vectors-etc.c
//
extern Val  allocate_nonempty_vector_of_eight_byte_floats__may_heapclean(Task* task,  int len,         Roots*);	// allocate_nonempty_vector_of_eight_byte_floats__may_heapclean	def in   src/c/heapcleaner/make-strings-and-vectors-etc.c
extern Val  allocate_biwordslots_vector__may_heapclean			(Task* task,  int len, 	       Roots*);	// allocate_biwordslots_vector__may_heapclean			def in   src/c/heapcleaner/make-strings-and-vectors-etc.c
extern Val  allocate_biwordslots_vector_sized_in_bytes__may_heapclean	(Task* task,  int len,         Roots*);	// allocate_biwordslots_vector_sized_in_bytes__may_heapclean	def in   src/c/heapcleaner/make-strings-and-vectors-etc.c
extern Val  make_biwordslots_vector_sized_in_bytes__may_heapclean	(Task* task,  void* data, int, Roots*);	// make_biwordslots_vector_sized_in_bytes__may_heapclean	def in   src/c/heapcleaner/make-strings-and-vectors-etc.c
//
extern Val  make_system_constant__may_heapclean				(Task* task,  Sysconsts*, int, Roots*);	// make_system_constant__may_heapclean				def in   src/c/heapcleaner/make-strings-and-vectors-etc.c
extern Val  dump_table_as_system_constants_list__may_heapclean		(Task* task,  Sysconsts*,      Roots*);	// dump_table_as_system_constants_list__may_heapclean		def in   src/c/heapcleaner/make-strings-and-vectors-etc.c

extern Val  make_package_literals_via_bytecode_interpreter__may_heapclean (Task* task, Unt8*, int len, Roots*);	// make_package_literals_via_bytecode_interpreter__may_heapclean def in
														//     src/c/heapcleaner/make-package-literals-via-bytecode-interpreter.c


extern Val zero_length_string__global [];
extern Val zero_length_vector__global [];

#define ZERO_LENGTH_STRING__GLOBAL	PTR_CAST( Val,  zero_length_string__global +1 )
#define ZERO_LENGTH_VECTOR__GLOBAL	PTR_CAST( Val,  zero_length_vector__global +1 )

#endif // RUNTIME_HEAP_H


////////////////////////////////
// Note[1]: Heap record allocation
//
// The general protocol for allocating (say)
// a seven-slot record in agegroup0 is
//
//     set_slot_in_nascent_heapchunk( task, 0, MAKE_TAGWORD(PAIRS_AND_RECORDS_BTAG, 7) );	// '7' is the record length in slots, not counting tagword.
//     set_slot_in_nascent_heapchunk( task, 1, val1 );
//     set_slot_in_nascent_heapchunk( task, 2, val2 );
//     set_slot_in_nascent_heapchunk( task, 3, val3 );
//     set_slot_in_nascent_heapchunk( task, 4, val4 );
//     set_slot_in_nascent_heapchunk( task, 5, val5 );
//     set_slot_in_nascent_heapchunk( task, 6, val6 );
//     set_slot_in_nascent_heapchunk( task, 7, val7 );
//     r =  commit_nascent_heapchunk( task, 7 );						// '7' is again the record length in slots not counting tagword.
//
// Here the
//     set_slot_in_nascent_heapchunk()
// calls write to unclaimed agegroup0 pace; only the final
//     commit_nascent_heapchunk()
// actually allocates heapspace by advancing
//     task->heap_allocation_pointer.
// 'r', the result value, points to the word *after* the
// tagword;  this is the conventional way of referring to
// such a record.
//
// AVOIDING AGEGROUP0 BUFFER OVERRUN:
//
//     Mythryl tries to guarantee a minimum of a thousand words
//     or so of free space in agegroup0 at all times, so you					// See MIN_FREE_BYTES_IN_AGEGROUP0_BUFFER in   src/c/h/runtime-configuration.h
//     can usually get away with allocating a few records without
//     checking available freespace, but if you are allocating more 
//     than a record or two you should check that agegroup0 has
//     sufficient space by doing one of
//
//         if (agegroup0_freespace_in_bytes(task) < bytes_needed) {
//         if (agegroup0_freespace_in_words(task) < words_needed) {
//
//     and then if not doing a minor heapcleaning.  The simple call
//     for this is
//
//         call_heapcleaner( task, 0 );								// '0' arg means clean out agegroup0 but do no further work unless logically required.
//
//     WARNING!!  This call may arbitrarily relocate the contents of
//     the heap, so you cannot use it if you have pointers into the
//     the heap, such as Val arguments to your fn.  In this case you
//     must instead use the call
//
//         call_heapcleaner_with_extra_roots( task, 0, &val1, &val2, ... &valn, NULL );
//
//     where val1, val2, ... valn are your retained Val pointers into
//     the heap.
//
//     If you are maintaining an unbounded number of such values just
//     string them together on a List and hand that list to
//     call_heapcleaner_with_extra_roots() -- for examples of that
//     see
//         make_nonempty_ro_vector__may_heapclean				in   src/c/heapcleaner/make-strings-and-vectors-etc.c 
//     or  make_package_literals_via_bytecode_interpreter__may_heapclean	in   src/c/heapcleaner/make-package-literals-via-bytecode-interpreter.c
//

// COPYRIGHT (c) 1992 AT&T Bell Laboratories
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2013,
// released per terms of SMLNJ-COPYRIGHT.


