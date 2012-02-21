// make-package-literals-via-bytecode-interpreter.c


// Problem
// =======
//
// When we generate a
//
//     foo.pkg.compiled
//
// file, we must somehow preserve foo.pkg's various
// literals and values -- which may include lists, records,
// tuples, trees etc -- on disk in a form allowing reconstitution
// when foo.pkg.compiled is later loaded into a running process.
//
// Solution
// ========
//
// We represent the literals as a bytecode program which
// when executed constructs the required values.  That is
// the job of the
//
//     Val   make_package_literals_via_bytecode_interpreter   (Task* task,   Unt8* bytecode_vector,   int bytecode_vector_bytesize)
//
// function in this file.  Our
//
//    bytecode_vector
//    bytecode_vector_bytesize
//
// arguments give the bytecode vector to execute,
// which was generated in
//
//     src/lib/compiler/back/top/main/make-nextcode-literals-bytecode-vector.pkg

#include "../mythryl-config.h"

#include <string.h>

#include "runtime-configuration.h"
#include "runtime-base.h"
#include "make-strings-and-vectors-etc.h"
#include "heap.h"



////////////////////////////////////////////////////////////////////////////////////////////
// Codes for bytecode machine instructions (version 1, Oct 22 1998):
//
//   Symbolic form                      Hex encoding
//   ============================       =================
//
//   MAKE_TAGGED_VAL(i)			0x01 <i>			// Inpointer values. These are 31-bits on 32-bit machines, 63-bits on 64-bit machines. Usually interpreted as ints, but that is a higher-level issue.
//
//   MAKE_FOUR_BYTE_VAL[i]		0x02 <i>			// Boxed four-byte nonpointer value, typically interpreted as unsigned or signed int.
//   MAKE_FOUR_BYTE_VALS[i1,..,in]	0x03 <n> <i1> ... <in>		// Make FOUR_BYTE_ALIGNED_NONPOINTER_DATA_BTAG  vector, filling it with data read directly from bytevector.
//
//   MAKE_EIGHT_BYTE_VAL[r]		0x04 <r>			// Boxed eight-byte nonpointer value, typically interpreted as a 64-bit float, but maybe as a 64-bit int or bitvector or whatever -- higher-level issue. 
//   MAKE_EIGHT_BYTE_VALS[r1,..,rn]	0x05 <n> <r1> ... <rn>		// Make EIGHT_BYTE_ALIGNED_NONPOINTER_DATA_BTAG vector, filling it with data read directly from bytevector.
//
//   MAKE_ASCII_STRING[c1,..,cn]	0x06 <n> <c1> ... <cn>		// Make bytevector.  This will typically be interpreted as a null-terminated ascii string, but that is a higher-level issue.
//   GET_ITH_LITERAL(k)			0x07 <k>			// Return a pointer to ith-last literal generated.  Useful when same literal value is referenced multiple times.
//
//   MAKE_VECTOR(n)			0x08 <n>			// Make vector of pointer values. Initialize it by popping values off our list of generated literals.
//   MAKE_RECORD(n)			0x09 <n>			// Make record of pointer values. Initialize it by popping values off our list of generated literals.
//
//   RETURN_LAST_LITERAL		0xff				// Pop top value off our list of generated literals and return it;  rest of list is discarded.
//
// NB: There is no MAKE_LIST.  I presume Lists are generated by a
//     sequence of MAKE_RECORD(2) instructions.   -- 2012-02-20 CrT

// These MUST be kept in sync with the bytecode generation logic in
//
//     src/lib/compiler/back/top/main/make-nextcode-literals-bytecode-vector.pkg
//
#define V1_MAGIC		0x19981022						// Generated by put_magic	in    src/lib/compiler/back/top/main/make-nextcode-literals-bytecode-vector.pkg
#define MAKE_TAGGED_VAL		0x01							// Generated by put_int		in    src/lib/compiler/back/top/main/make-nextcode-literals-bytecode-vector.pkg
//
#define MAKE_FOUR_BYTE_VAL	0x02							// Generated by put_raw32	in    src/lib/compiler/back/top/main/make-nextcode-literals-bytecode-vector.pkg
#define MAKE_FOUR_BYTE_VALS	0x03							// Generated by put_raw32	in    src/lib/compiler/back/top/main/make-nextcode-literals-bytecode-vector.pkg
//
#define MAKE_EIGHT_BYTE_VAL	0x04							// Generated by put_raw64	in    src/lib/compiler/back/top/main/make-nextcode-literals-bytecode-vector.pkg
#define MAKE_EIGHT_BYTE_VALS	0x05							// Generated by put_raw64	in    src/lib/compiler/back/top/main/make-nextcode-literals-bytecode-vector.pkg
//
#define MAKE_ASCII_STRING	0x06							// Generated by put_string	in    src/lib/compiler/back/top/main/make-nextcode-literals-bytecode-vector.pkg
#define GET_ITH_LITERAL		0x07							// Generated by put_lit		in    src/lib/compiler/back/top/main/make-nextcode-literals-bytecode-vector.pkg
//
#define MAKE_VECTOR		0x08							// Generated by put_vector	in    src/lib/compiler/back/top/main/make-nextcode-literals-bytecode-vector.pkg
#define MAKE_RECORD		0x09							// Generated by put_record	in    src/lib/compiler/back/top/main/make-nextcode-literals-bytecode-vector.pkg
//
#define RETURN_LAST_LITERAL	0xff							// Generated by put_return	in    src/lib/compiler/back/top/main/make-nextcode-literals-bytecode-vector.pkg



// Fetch a 32-bit int from address 'p' in big-endian order:				// 64-bit issue.
// 
#define GET32(p,pc)	\
    ( (p[pc+0] << 24)	\
    | (p[pc+1] << 16)	\
    | (p[pc+2] <<  8)	\
    | (p[pc+3] <<  0)	\
    )

#define LIST_CONS_CELL_BYTESIZE	(WORD_BYTESIZE*3)		// Size of a list cons cell in bytes.



static double   get_double   (Unt8* p)   {
    //          ==========
    //
    union {
	double	d;
	Unt8	b[ sizeof( double ) ];
    } u;

    #ifdef BYTE_ORDER_LITTLE
	//
	for (int i = sizeof(double);   i --> 0;  ) {
	    //
	    u.b[i] =  *p++;
	}
    #else
	for (int i = 0;   i < sizeof(double);   i++) {
	    //
	    u.b[i] = p[i];
	}
    #endif

    return u.d;
}

static int   empty_agegroup0_buffer_if_more_than_half_full   (Task* task,  Roots* extra_roots)   {
    //       =============================================
    //
    // The original SML/NJ code tried to empty the
    // agegroup0 buffer only if it was absolutely full.
    //
    // The original SML/NJ code also tended to segfault
    // and coredump under obscure conditions hard to
    // repeat or diagnose.
    //
    // I'm not as fond of Heisenbugs as the SML/NJ Fellowship
    // apparently is, so here I'm erring on the side of being
    // very, very conservative.
    //
    // It should be sufficient to empty the agegroup0 buffer
    // only if it is within about
    //
    //     MAX_AGEGROUP0_ALLOCATION_SIZE_IN_WORDS			// about 2KB -- see src/c/h/runtime-configuration.h
    //
    // of full, and the agegroup0 buffer is about 256KB long		// See DEFAULT_AGEGROUP0_BUFFER_BYTESIZE in src/c/h/runtime-configuration.h
    // so emptying when over half-full is conservative.  But
    // we don't get called very often, and at worst we call
    // the garbage collector twice as often as usual (normally
    // about 200Hz)  -- which does *not* mean it does twice as
    // much work, since it does work only proportional to live
    // data in the buffer -- so why not just stay a really long
    // way away from the slightest risk of buffer-overrun here?
    //
    int freebytes = agegroup0_freespace_in_bytes(task);
    int usedbytes = agegroup0_usedspace_in_bytes(task);

    if (freebytes < usedbytes) {
	//
	call_heapcleaner_with_extra_roots (task, 0, extra_roots );	// '0' means only empty agegroup0.
    }
    return freebytes;
}

Val   make_package_literals_via_bytecode_interpreter__may_heapclean   (Task* task,   Unt8* bytecode_vector,   int bytecode_vector_bytesize,  Roots* extra_roots)   {
    //=============================================================
    //
    // bytecode_vector is a Mythryl-heap vector datachunk cast to Unt8*.
    //
    // NOTE: We allocate all of the chunks in agegroup 1,
    // but allocate the vector of literals in agegroup0.
    //
    // We get called at the C level in
    //
    //    src/c/main/load-compiledfiles.c
    //
    // This fn gets exported to the Mythryl level as
    //
    //     make_package_literals_via_bytecode_interpreter
    // in
    //     src/lib/compiler/execution/code-segments/code-segment.pkg
    // via
    //     src/c/lib/heap/libmythryl-heap.c
    //
    // Our ultimate Mythryl-level invocation is in
    //
    //     src/lib/compiler/execution/main/link-and-run-package.pkg

    Val	stack = HEAP_NIL;

    Roots roots1 = { (Val*)&bytecode_vector, extra_roots };
    Roots roots2 = { &stack,                &roots1	 };

								do_debug_logging =  TRUE;
								check_agegroup0_overrun_tripwire_buffer( task, "make_package_literals_via_bytecode_interpreter__may_heapclean/AAA" );


    int pc = 0;														// 'pc' will be our 'program counter' offset into bytecode_vector.


    if (bytecode_vector_bytesize <= 8)   return HEAP_NIL;								// bytecode_vector has an 8-byte header, so length <= 8 means nothing to do.

    Val_Sized_Unt  magic
	=
	GET32(bytecode_vector,pc);   pc += 4;

    Val_Sized_Unt  max_depth												/* This var is currently unused, so suppress 'unused var' compiler warning: */ 		__attribute__((unused))
	=
	GET32(bytecode_vector,pc);   pc += 4;

    if (magic != V1_MAGIC)   die("bogus literal magic number %#x", magic);


Val_Sized_Int* tripwirebuf = (Val_Sized_Int*) (((char*)(task->real_heap_allocation_limit)) + MIN_FREE_BYTES_IN_AGEGROUP0_BUFFER);

    for (;;) {
	int free_bytes													/* This var is currently unused, so suppress 'unused var' compiler warning: */ 		__attribute__((unused))
	    =
	    empty_agegroup0_buffer_if_more_than_half_full( task, &roots2 );

	ASSERT(pc < bytecode_vector_bytesize);

if (tripwirebuf[0] != 0) log_if("luptop TRIPWIRE BUFFER TRASHED!");

	switch (bytecode_vector[ pc++ ]) {
	    //
	case MAKE_TAGGED_VAL:												// Make 31-bit in-pointer int on 32-bit machines, 63-bit in-pointer in on 64-bit machines.
	    {
		int i = GET32(bytecode_vector,pc);	pc += 4;							// 64-bit issue.

		stack = LIST_CONS(task, TAGGED_INT_FROM_C_INT(i), stack);						// LIST_CONST		is from   src/c/h/make-strings-and-vectors-etc.h
	    }
	    break;

	case MAKE_FOUR_BYTE_VAL:											// Make a boxed 32-bit int or unt.   This will consume two words, one for the tagword, one for the value.
	    {
		int i = GET32(bytecode_vector,pc);	pc += 4;							// 64-bit issue.

		Val result =  make_one_word_int(task, i );								// make_one_word_int	is from   src/c/h/make-strings-and-vectors-etc.h

		stack = LIST_CONS(task, result, stack);									// Add boxed int/unt to 'stack'.  This will consume three words -- tagword plus the head and tail pointers.
	    }
	    break;

	case MAKE_FOUR_BYTE_VALS:											// Make vector of word-length (4byte?) nonpointer values. We'll use one tagword + N words for the values.
	    {
		int n = GET32(bytecode_vector,pc);	pc += 4;							// 64-bit issue.

		ASSERT(n > 0);

		set_slot_in_nascent_heapchunk
		  ( task,
                    0,
                    MAKE_TAGWORD(n, FOUR_BYTE_ALIGNED_NONPOINTER_DATA_BTAG)						// 64-bit issue.
		);

		for (int j = 1;  j <= n;  j++) {
		    //
		    int i = GET32(bytecode_vector,pc);	pc += 4;							// 64-bit issue.

		    set_slot_in_nascent_heapchunk (task, j, (Val)i);
		}

		Val result =  commit_nascent_heapchunk(task, n );

		stack = LIST_CONS(task, result, stack);
	    }
	    break;

	case MAKE_EIGHT_BYTE_VAL:											// Make a boxed 64-bit nonpointer value -- usually a float, potentially a 64-bit int/unt/whatever.
	    {														// On 32-bit machines this will consume up to four words: one for alignment padding, two for data, one tagword.
															// On 64-bit machines this will consume exactly two words: tagword plus dataword.
		double d = get_double(&(bytecode_vector[pc]));	pc += 8;						// 64-bit issue.

		Val result = make_float64(task, d );									// make_float64	def in   src/c/h/make-strings-and-vectors-etc.h

		stack = LIST_CONS(task, result, stack);									// Add 'result' to 'stack'.  This will consume three words -- tagword plus the head and tail pointers.
	    }
	    break;

	case MAKE_EIGHT_BYTE_VALS:											// Make vector of biword-length (8byte?) nonpointer values. We'll use one tagword + N words for the values.
	    {														// Currently this will (almost?) always be float64s.
		int len_in_slots = GET32(bytecode_vector,pc);								// Get of 64-bit slots for vector.  (Why HALF??)
		pc += 4;												// 64-bit issue -- on 64-bit machines should probably be 8 bytes.

		ASSERT(n > 0);

		#ifdef ALIGN_FLOAT64S
		    // Force FLOAT64_BYTESIZE alignment (descriptor is off by one word)
		    //
		    task->heap_allocation_pointer = (Val*)((Punt)(task->heap_allocation_pointer) | WORD_BYTESIZE);
		#endif

		Val result;

		{   int len_in_hostwords = 2 * len_in_slots;								// Compute length for tagword -- tagword length is in hostwords not 64-bit words.	// 64-bit issue.

		    set_slot_in_nascent_heapchunk
		      ( task,
			0,
			MAKE_TAGWORD( len_in_hostwords, EIGHT_BYTE_ALIGNED_NONPOINTER_DATA_BTAG)
		      );

		    result =  commit_nascent_heapchunk(task, len_in_hostwords );
		}

		for (int i = 0;  i < len_in_slots;  i++) {
		    //
		    PTR_CAST(double*, result)[i] = get_double(&(bytecode_vector[pc]));	pc += 8;			// 64-bit issue.
		}

		stack = LIST_CONS(task, result, stack);									// Add 'result' to 'stack'.  This will consume three words -- tagword plus the head and tail pointers.
	    }
	    break;

	case MAKE_ASCII_STRING:												// More generally, vector of byte-size values.
	    {														// We allocate a vector and also the silly separate vector-header,
															// so total space consumption is two tagwords, indirection pointer, plus the bytevector contents proper.
		int len_in_bytes = GET32(bytecode_vector,pc);		pc += 4;					// 64-bit issue.

		if (len_in_bytes == 0) {
		    //
		    stack = LIST_CONS(task, ZERO_LENGTH_STRING__GLOBAL, stack);

		} else {

		    Val result; 	

		    {	int  len_in_hostwords
			    =
			    BYTES_TO_WORDS( len_in_bytes +1 );								// '+1' to include space for '\0'.

			set_slot_in_nascent_heapchunk									// Write the bytevector tagword.
			  ( task,
			    0,
			    MAKE_TAGWORD( len_in_hostwords, FOUR_BYTE_ALIGNED_NONPOINTER_DATA_BTAG)
			);

			set_slot_in_nascent_heapchunk     (task, len_in_hostwords, 0);					// Make sure any left-over bytes are zeroed, so word-by-word string equality works.
			result = commit_nascent_heapchunk (task, len_in_hostwords);
		    }	

		    memcpy (PTR_CAST(void*, result), &bytecode_vector[pc], len_in_bytes);	pc += len_in_bytes;

		    result = make_vector_header(task, STRING_TAGWORD, result, len_in_bytes);				// Allocate the header chunk.

		    stack = LIST_CONS(task, result, stack);								// Add 'result' to 'stack'.  This will consume three words -- tagword plus the head and tail pointers.
		}
	    }
	    break;

	case GET_ITH_LITERAL:												// Get i-th literal from list of already-constructed values.
	    {

		int n = GET32(bytecode_vector,pc);	pc += 4;							// 64-bit issue.

		Val result = stack;

		for (int i = 0;  i < n;  ++i) {
		    //
		    result = LIST_TAIL( result );
		}

		stack = LIST_CONS(task, LIST_HEAD(result), stack);							// Add 'result' to 'stack'.  This will consume three words -- tagword plus the head and tail pointers.
	    }
	    break;

	  case MAKE_VECTOR:												// Make n-slot vector, fill it by popping last N values from stack.
	    {														// We allocate the vector and also the silly separate vector-header,
															// so total space consumption is two tagwords, indirection pointer, plus the bytevector contents proper.

		int len_in_slots = GET32(bytecode_vector,pc);	pc += 4;						// 64-bit issue.

		if (len_in_slots == 0) {
		    //
		    stack = LIST_CONS(task, ZERO_LENGTH_VECTOR__GLOBAL, stack);
		    //
		} else {
		    //
		    set_slot_in_nascent_heapchunk(task, 0, MAKE_TAGWORD(len_in_slots, RO_VECTOR_DATA_BTAG));		// Do tagword for vector.		// 64-bit issue?

		    // Over all slots in vector:
		    //
		    for (int i = len_in_slots;  i > 0;  --i) {								// Iterate in reverse order because top of stack is last element in record.
			//
			set_slot_in_nascent_heapchunk(task, i, LIST_HEAD(stack));					// Initialize i-th slot of vector.

			stack = LIST_TAIL( stack );									// Pop slot initializer from stack.
		    }

		    Val result =  commit_nascent_heapchunk(task, len_in_slots );					// Allocate the data chunk.

		    result =  make_vector_header(task, TYPEAGNOSTIC_RO_VECTOR_TAGWORD, result, len_in_slots );		// Allocate the silly indirect-reference-to-vector record.

		    stack = LIST_CONS(task, result, stack);								// Add 'result' to 'stack'.  This will consume three words -- tagword plus the head and tail pointers.
		}
	    }
	    break;

	case MAKE_RECORD:												// Make n-slot record, fill it by popping last N values from stack.
	    {

		int len_in_slots = GET32(bytecode_vector,pc);	pc += 4;						// 64-bit issue.

		if (len_in_slots == 0) {
		    //
		    stack = LIST_CONS(task, HEAP_VOID, stack);
		    //
		} else {

		    set_slot_in_nascent_heapchunk(task, 0, MAKE_TAGWORD(len_in_slots, PAIRS_AND_RECORDS_BTAG));		// Set up tagword for vector.		// 64-bit issue?

		    // Over all slots in record:
		    //
		    for (int i = len_in_slots;  i > 0;  i--) {								// Iterate in reverse order because top of stack is last element in record.
			//
			set_slot_in_nascent_heapchunk(task,  i,  LIST_HEAD(stack));					// Set up i-th slot in vector.

			stack = LIST_TAIL(stack);									// Pop slot initializer from stack.
		    }

		    Val result = commit_nascent_heapchunk(task, len_in_slots );						// Allocate the vector by bumping end-of-buffer pointer.

		    stack = LIST_CONS(task, result, stack);								// Add 'result' to 'stack'.  This will consume three words -- tagword plus the head and tail pointers.
		}

	    }
	    break;

	case RETURN_LAST_LITERAL:											// Pop and return top entry on linklist of constructed literals, discarding rest of list.
	    //	
	    ASSERT(pc == bytecode_vector_bytesize);

								check_agegroup0_overrun_tripwire_buffer( task, "make_package_literals_via_bytecode_interpreter__may_heapclean/ZZZ" );
	    return  LIST_HEAD( stack );
	    break;

	default:
	    die ("bogus literal opcode #%x @ %d", bytecode_vector[pc-1], pc-1);
	}								// switch
    }									// while

    do_debug_logging =  FALSE;
}									// fun make_package_literals_via_bytecode_interpreter__may_heapclean


//////////////////////////////////////////////////////////////////////////////////////////////
// Historical note:
//
// APPARENT SEGFAULT BUG -- 2012-01-02 CrT
//
// Mythryl segfaults every now and then while "linking" and this module
// appears to be the reason.  What appears to be happening is that the
// logic here assumes that
//    (1) Every heapcleaner call leaves at least 64KB free in gen0.
//    (2) Strings are never bigger than 64K.
// From examining the logging currently enabled, it appears that heapcleaning
// leaves more like 8KB free in gen0, and that some strings are in fact bigger
// than 64KB.
//    [ Later (2012-01-31):  From a careful reading of  src/c/heapcleaner/heapclean-agegroup0.c
//      it is clear that a minor heapclean will necessarily always leave agegroup0 completely
//      empty -- meaning circa 256KB free -- so I need to investigate why it appears to leave
//      8K or so sometimes.  Prosumably broken reporting.
//    ]
// We have a fn   check_agegroup0_overrun_tripwire_buffer      in   src/c/heapcleaner/heap-debug-stuff.c
// We have a fn   partition_agegroup0_buffer_between_pthreads  in   src/c/heapcleaner/pthread-heapcleaner-stuff.c
// To add flavor to the mix:
//    *  In src/c/heapcleaner/make-strings-and-vectors-etc.c
//       we seem to make a point of never allocating more than MAX_AGEGROUP0_ALLOCATION_SIZE_IN_WORDS
//       words in gen0, but in this file we appear to make no such effort.
//    *  Doing a two-generation heapcleaning should resolve the problem, but
//       in fact segfaults us -- search for [XYZZY].
//////////////////////////////////////////////////////////////////////////////////////////////



// COPYRIGHT (c) 1997 Bell Labs, Lucent Technologies.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.




/*
##########################################################################
#   The following is support for outline-minor-mode in emacs.		 #
#  ^C @ ^T hides all Text. (Leaves all headings.)			 #
#  ^C @ ^A shows All of file.						 #
#  ^C @ ^Q Quickfolds entire file. (Leaves only top-level headings.)	 #
#  ^C @ ^I shows Immediate children of node.				 #
#  ^C @ ^S Shows all of a node.						 #
#  ^C @ ^D hiDes all of a node.						 #
#  ^HFoutline-mode gives more details.					 #
#  (Or do ^HI and read emacs:outline mode.)				 #
#									 #
# Local variables:							 #
# mode: outline-minor							 #
# outline-regexp: "[A-Za-z]"			 		 	 #
# End:									 #
##########################################################################
*/

