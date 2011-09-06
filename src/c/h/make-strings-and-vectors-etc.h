// make-strings-and-vectors-etc.h
//
// Macros and routines for allocating heap chunks.


#ifndef RUNTIME_HEAP_H
#define RUNTIME_HEAP_H

#include "runtime-base.h"
#include "runtime-values.h"
#include "task.h"
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


////////////////////////////////
// Heap allocation macros

#define LIB7_AllocWrite(task, i, x)	((((task)->heap_allocation_pointer))[i] = (x))

#define LIB7_Alloc(task, n)	(			\
    ((task)->heap_allocation_pointer += ((n)+1)),			\
    PTR_CAST( Val, (task)->heap_allocation_pointer - (n)))

#define REF_ALLOC(task, r, a)	{			\
	Task	*__task = (task);			\
	Val	*__p = __task->heap_allocation_pointer;		\
	*__p++ = REFCELL_TAGWORD;			\
	*__p++ = (a);					\
	(r) = PTR_CAST( Val, __task->heap_allocation_pointer + 1);	\
	__task->heap_allocation_pointer = __p;			\
    }

#define REC_ALLOC1(task, r, a)	{				\
	Task	*__task = (task);				\
	Val	*__p = __task->heap_allocation_pointer;			\
	*__p++ = MAKE_TAGWORD(1, PAIRS_AND_RECORDS_BTAG);	\
	*__p++ = (a);						\
	(r) = PTR_CAST( Val, __task->heap_allocation_pointer + 1);		\
	__task->heap_allocation_pointer = __p;				\
    }

#define REC_ALLOC2(task, r, a, b)	{			\
	Task	*__task = (task);				\
	Val	*__p = __task->heap_allocation_pointer;			\
	*__p++ = PAIR_TAGWORD;					\
	*__p++ = (a);						\
	*__p++ = (b);						\
	(r) = PTR_CAST( Val, __task->heap_allocation_pointer + 1);		\
	__task->heap_allocation_pointer = __p;				\
    }

#define REC_ALLOC3(task, r, a, b, c)	{			\
	Task	*__task = (task);				\
	Val	*__p = __task->heap_allocation_pointer;			\
	*__p++ = MAKE_TAGWORD(3, PAIRS_AND_RECORDS_BTAG);		\
	*__p++ = (a);						\
	*__p++ = (b);						\
	*__p++ = (c);						\
	(r) = PTR_CAST( Val, __task->heap_allocation_pointer + 1);		\
	__task->heap_allocation_pointer = __p;				\
    }

#define REC_ALLOC4(task, r, a, b, c, d)	{			\
	Task	*__task = (task);				\
	Val	*__p = __task->heap_allocation_pointer;			\
	*__p++ = MAKE_TAGWORD(4, PAIRS_AND_RECORDS_BTAG);	\
	*__p++ = (a);						\
	*__p++ = (b);						\
	*__p++ = (c);						\
	*__p++ = (d);						\
	(r) = PTR_CAST( Val, __task->heap_allocation_pointer + 1);		\
	__task->heap_allocation_pointer = __p;				\
    }

#define REC_ALLOC5(task, r, a, b, c, d, e)	{		\
	Task	*__task = (task);				\
	Val	*__p = __task->heap_allocation_pointer;			\
	*__p++ = MAKE_TAGWORD(5, PAIRS_AND_RECORDS_BTAG);	\
	*__p++ = (a);						\
	*__p++ = (b);						\
	*__p++ = (c);						\
	*__p++ = (d);						\
	*__p++ = (e);						\
	(r) = PTR_CAST( Val, __task->heap_allocation_pointer + 1);		\
	__task->heap_allocation_pointer = __p;				\
    }

#define REC_ALLOC6(task, r, a, b, c, d, e, f)	{		\
	Task	*__task = (task);				\
	Val	*__p = __task->heap_allocation_pointer;			\
	*__p++ = MAKE_TAGWORD(6, PAIRS_AND_RECORDS_BTAG);	\
	*__p++ = (a);						\
	*__p++ = (b);						\
	*__p++ = (c);						\
	*__p++ = (d);						\
	*__p++ = (e);						\
	*__p++ = (f);						\
	(r) = PTR_CAST( Val, __task->heap_allocation_pointer + 1);		\
	__task->heap_allocation_pointer = __p;				\
    }

#define SEQHDR_ALLOC(task, r, desc, data, len)	{		\
	Task	*__task = (task);				\
	Val	*__p = __task->heap_allocation_pointer;			\
	*__p++ = (desc);					\
	*__p++ = (data);					\
	*__p++ = TAGGED_INT_FROM_C_INT(len);				\
	(r) = PTR_CAST( Val, __task->heap_allocation_pointer + 1);		\
	__task->heap_allocation_pointer = __p;				\
    }

#ifdef ALIGN_FLOAT64S
#define REAL64_ALLOC(task, r, d) {				\
	Task	*__task = (task);				\
	Val	*__p = __task->heap_allocation_pointer;			\
	__p = (Val *)((Punt)__p | WORD_BYTESIZE);		\
	*__p++ = FLOAT64_TAGWORD;				\
	(r) = PTR_CAST( Val, __p);				\
	*(double *)__p = (d);					\
	__p += FLOAT64_SIZE_IN_WORDS;				\
	__task->heap_allocation_pointer = __p;				\
    }
#else
#define REAL64_ALLOC(task, r, d) {				\
	Task	*__task = (task);				\
	Val	*__p = __task->heap_allocation_pointer;			\
	__p = (Val *)((Punt)__p | WORD_BYTESIZE);		\
	(r) = PTR_CAST( Val, __p);				\
	*(double *)__p = (d);					\
	__p += FLOAT64_SIZE_IN_WORDS;				\
	__task->heap_allocation_pointer = __p;				\
    }
#endif

#define EXN_ALLOC(task, ex, id, val, where) \
	REC_ALLOC3(task, ex, id, val, where)

// Boxed word values
//
#define WORD_LIB7toC(w)		(*PTR_CAST(Val_Sized_Unt*, w))
#define WORD_ALLOC(task, p, w)	{				\
	Task	*__task = (task);				\
	Val	*__p = __task->heap_allocation_pointer;			\
	*__p++ = MAKE_TAGWORD(1, FOUR_BYTE_ALIGNED_NONPOINTER_DATA_BTAG);			\
	*__p++ = (Val)(w);					\
	(p) = PTR_CAST( Val, __task->heap_allocation_pointer + 1);		\
	__task->heap_allocation_pointer = __p;				\
    }
#define TUPLE_GETWORD(p, i)	(*GET_TUPLE_SLOT_AS_PTR(Val_Sized_Unt*, p, i))
#define INT1_LIB7toC(i)		(*PTR_CAST(Int1*, i))
#define INT1_ALLOC(task, p, i)	WORD_ALLOC(task, p, i)
#define TUPLE_GET_INT1(p, i)	(*GET_TUPLE_SLOT_AS_PTR(Int1*, p, i))


//////////////////
// Mythryl lists
//
#define LIST_HEAD(p)			GET_TUPLE_SLOT_AS_VAL(p, 0)
#define LIST_TAIL(p)			GET_TUPLE_SLOT_AS_VAL(p, 1)
#define LIST_CONS(task, r, a, b)	REC_ALLOC2(task, r, a, b)
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
#define OPTION_THE(task, r, a)		REC_ALLOC1(task, r, a)
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
extern Val  make_ascii_string_from_c_string			(Task* task,  const char* string);			// make_heap_string_from_c_string		def in   src/c/cleaner/make-strings-and-vectors-etc.c
extern Val  make_ascii_strings_from_vector_of_c_strings		(Task* task,  char** strings);				// make_ascii_strings_from_vector_of_c_strings	def in   src/c/cleaner/make-strings-and-vectors-etc.c
extern Val  allocate_nonempty_ascii_string			(Task* task,  int len);					// allocate_nonempty_ascii_string		def in   src/c/cleaner/make-strings-and-vectors-etc.c
extern Val  allocate_nonempty_vector_of_one_byte_unts			(Task* task,  int len);					// allocate_nonempty_vector_of_one_byte_unts		def in   src/c/cleaner/make-strings-and-vectors-etc.c
//
extern Val  allocate_nonempty_code_chunk			(Task* task,  int len);					// allocate_nonempty_code_chunk			def in   src/c/cleaner/make-strings-and-vectors-etc.c
//
extern Val  make_nonempty_rw_vector				(Task* task,  int len, Val initial_value);		// make_nonempty_rw_vector			def in   src/c/cleaner/make-strings-and-vectors-etc.c
extern Val  make_nonempty_ro_vector				(Task* task,  int len, Val initial_values);		// make_nonempty_ro_vector			def in   src/c/cleaner/make-strings-and-vectors-etc.c
extern Val  allocate_nonempty_int1_vector		        (Task* task,  int length_in_words);			// allocate_nonempty_int1_vector		def in   src/c/cleaner/make-strings-and-vectors-etc.c
extern void shrink_fresh_int1_vector				(Task* task,  Val v, int new_length_in_words);		// shrink_fresh_int1_vector			def in   src/c/cleaner/make-strings-and-vectors-etc.c
//
extern Val  allocate_nonempty_vector_of_eight_byte_floats	(Task* task,  int len);					// allocate_nonempty_vector_of_eight_byte_floats		def in   src/c/cleaner/make-strings-and-vectors-etc.c
extern Val  allocate_int2_vector				(Task* task,  int length_in_int2s);			// allocate_int2_vector			def in   src/c/cleaner/make-strings-and-vectors-etc.c
extern Val  allocate_int2_vector_sized_in_bytes  		(Task* task,  int length_in_bytes/*gets rounded up*/);	// allocate_int2_vector_sized_in_bytes		def in   src/c/cleaner/make-strings-and-vectors-etc.c
extern Val  make_int2_vector_sized_in_bytes        		(Task* task,  void* data, int nbytes);			// make_int2_vector_sized_in_bytes		def in   src/c/cleaner/make-strings-and-vectors-etc.c
//
extern Val  make_system_constant				(Task* task,  System_Constants_Table* table, int id);	// make_system_constant				def in   src/c/cleaner/make-strings-and-vectors-etc.c
extern Val  dump_table_as_system_constants_list			(Task* task,  System_Constants_Table* table);		// dump_table_as_system_constants_list		def in   src/c/cleaner/make-strings-and-vectors-etc.c

extern Val make_package_literals_via_bytecode_interpreter (Task* task,  Unt8* lits,  int len);

extern Val zero_length_string_global [];
extern Val zero_length_vector_global [];

#define ZERO_LENGTH_STRING_GLOBAL	PTR_CAST( Val,  zero_length_string_global +1 )
#define ZERO_LENGTH_VECTOR_GLOBAL	PTR_CAST( Val,  zero_length_vector_global +1 )

#endif // RUNTIME_HEAP_H


// COPYRIGHT (c) 1992 AT&T Bell Laboratories
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.


