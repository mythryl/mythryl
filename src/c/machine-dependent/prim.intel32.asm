// prim.intel32.asm
//
// This file contains asmcoded functions callable directly
// from Mythryl via the runtime::asm API defined in
//
//     src/lib/core/init/runtime.api
//
// These assembly code functions may then request services from
// the C level by passing one of the request codes defined in
//
//     src/c/h/asm-to-c-request-codes.h
//
// to
//
//     src/c/main/run-mythryl-code-and-runtime-eventloop.c
//
// which then dispatches on them to various fns throughout the C runtime.
//
// This was derived from I386.prim.s, by Mark Leone (mleone@cs.cmu.edu)
//
// Completely rewritten and changed to use assyntax.h, by Lal George.


/*
###             "He who hasn't hacked assembly
###              language as a youth has no heart.
###              He who does as an adult has no brain."
###
###                           --- John Moore
*/

		
#include "assyntax.h"
#include "runtime-base.h"
#include "asm-base.h"
#include "runtime-values.h"
#include "heap-tags.h"
#include "asm-to-c-request-codes.h"
#include "runtime-configuration.h"
	
#if defined(OPSYS_DARWIN)
    //
    // Note: although the MacOS assembler claims to be the GNU assembler,
    // it appears to be an old version (1.38), which uses different
    // alignment directives.
    //	
    #undef ALIGNTEXT4
    #undef ALIGNDATA4
    #define ALIGNTEXT4	.align 2
    #define ALIGNDATA4	.align 2
#endif


// The 386 registers are used as follows:
//
// EAX - temp1 (see the code generator, intel32/intel32.pkg)
// EBX - misc0
// ECX - misc1
// EDX - misc2
// ESI - standard fate (fate, see task.h)
// EBP - standard argument (argument)
// EDI - free space pointer (heap_allocation_pointer)
// ESP - stack pointer
// EIP - program counter (program_counter)


// Stack frame:
//
#define temp			EAX
#define misc0			EBX
#define misc1			ECX
#define misc2			EDX
#define stdfate			ESI				// Needs to match   stdfate		     in   src/lib/compiler/back/low/main/intel32/backend-lowhalf-intel32-g.pkg
#define stdarg			EBP				// Needs to match   stdarg		     in   src/lib/compiler/back/low/main/intel32/backend-lowhalf-intel32-g.pkg

#define heap_allocation_pointer	EDI				// We allocate ram just by advancing this pointer.  We use this very heavily -- every 10 instructions or so.
								// Needs to match   heap_allocation_pointer  in   src/lib/compiler/back/low/main/intel32/backend-lowhalf-intel32-g.pkg

#define stackptr		ESP				// Needs to match   stackptr		     in   src/lib/compiler/back/low/main/intel32/backend-lowhalf-intel32-g.pkg

// Other register uses:	
//
#define creturn 	EAX

	// Stack frame.
#define tempmem			REGOFF(0,ESP)
#define baseptr			REGOFF(4,ESP)			// Needs to match   baseptr                  in   src/lib/compiler/back/low/main/intel32/backend-lowhalf-intel32-g.pkg
#define exnfate			REGOFF(8,ESP)			// Needs to match   exnptr		     in   src/lib/compiler/back/low/main/intel32/backend-lowhalf-intel32-g.pkg

#define heap_allocation_limit	REGOFF(12,ESP)			// heapcleaner gets run when heap_allocation_pointer reaches this point.
								// Needs to match   heap_allocation_limit    in   src/lib/compiler/back/low/main/intel32/backend-lowhalf-intel32-g.pkg

#define pc			REGOFF(16,ESP)			// Needs?to match   heapcleaner_link	     in   src/lib/compiler/back/low/main/intel32/backend-lowhalf-intel32-g.pkg

#define unused_1		REGOFF(20,ESP)

#define heap_changelog_ptr	REGOFF(24,ESP)			// Every (pointer) update to the heap gets logged to this heap-allocated cons-cell list.
								// (The heapcleaner scans this list to detect intergenerational pointers.)
								// Needs to match   heap_changelog_pointer   in   src/lib/compiler/back/low/main/intel32/backend-lowhalf-intel32-g.pkg

#define current_thread_ptr	REGOFF(28,ESP)

#define run_heapcleaner_ptr	REGOFF(32,ESP)			// Needs to match   run_heapcleaner__offset  in  src/lib/compiler/back/low/main/intel32/machine-properties-intel32.pkg
								// This ptr is used to invoke the heapcleaner by code generated in   src/lib/compiler/back/low/main/fatecode/insert-treecode-heapcleaner-calls-g.pkg
								// This ptr is set by asm_run_mythryl_task (below) to point to call_heapcleaner (below) which returns a REQUEST_CLEANING to
								// run_mythryl_task_and_runtime_eventloop ()  in   src/c/main/run-mythryl-code-and-runtime-eventloop.c
								// which will call   clean_heap	()            in   src/c/cleaner/call-cleaner.c
#define unused_2		REGOFF(36,ESP)
#define eaxSpill		REGOFF(40,ESP) // eax=0
#define	ecxSpill		REGOFF(44,ESP) // ecx=1
#define	edxSpill		REGOFF(48,ESP) // edx=2
#define	ebxSpill		REGOFF(52,ESP) // ebx=3
#define	espSpill		REGOFF(56,ESP) // esp=4
#define	ebpSpill		REGOFF(60,ESP) // ebp=5
#define	esiSpill		REGOFF(64,ESP) // esi=6
#define	ediSpill		REGOFF(68,ESP) // edi=7
#define stdlink			REGOFF(72,ESP)
#define	stdclos			REGOFF(76,ESP)

#define espsave		REGOFF(500,ESP)

#define task_offset 176								// 			Must    match   task_offset            in   src/lib/compiler/back/low/main/intel32/machine-properties-intel32.pkg
#define task_ptr	REGOFF(task_offset, ESP)
#define freg8           184							// Doubleword aligned
#define	freg9           192
#define freg31          368							// 152 + (31-8)*8
#define	fpTempMem	376							// freg31 + 8
#define SpillAreaStart	512							// Starting offset.	Must    match   initial_spill_offset   in   src/lib/compiler/back/low/main/intel32/machine-properties-intel32.pkg
#define LIB7_FRAME_SIZE	(8192)							// 			Must(?) match   spill_area_size        in   src/lib/compiler/back/low/main/intel32/machine-properties-intel32.pkg

#define	via

	SEG_DATA
	ALIGNDATA4
request_w:					// Place to put the request code.
	D_LONG 0
	GLOBL CSYM(LIB7_intel32Frame)
LABEL(CSYM(LIB7_intel32Frame)) 			// Pointer to the ml frame (gives C access to heap_allocation_limit)
	D_LONG 0		


#include "task-and-pthread-struct-field-offsets--autogenerated.h"


//
// 386 function call conventions:  
//  [true for gcc and dynix3 cc; untested for others]
//
// 	Caller save registers: eax, ecx, edx
// 	Callee save registers: ebx, esi, edi, and ebp.
//	Save frame pointer (ebx) first to match standard function prelude
// 	Floating point state is caller-save.
// 	Arguments passed on stack.  Rightmost argument pushed first.
// 	Word-sized result returned in %eax.
//	On Darwin, stack frame must be multiple of 16 bytes

#define cresult	EAX

#define CALLEE_SAVE_BYTESIZE 16	// ebp, ebx, esi, edi

#define CALLEE_SAVE	\
	PUSH_L(EBP);	\
	PUSH_L(EBX);	\
	PUSH_L(ESI);	\
	PUSH_L(EDI)

#define CALLEE_RESTORE	\
	POP_L(EDI);	\
	POP_L(ESI);	\
	POP_L(EBX);	\
	POP_L(EBP)

// MOVE copies one memory location to another, using a specified temporary.

#define MOVE(src,tmp,dest)	\
	MOV_L(src, tmp);	\
	MOV_L(tmp, dest)

#define CONTINUE				\
	JMP(CODEPTR(stdfate))

// 2011-02-01 CrT: I'm guessing 'JB(9f)'  branches to 9: with 'f' for 'forward',
//                 and that     'JMP(1b)' branches to 1: with 'b' for 'backward'.
#define CHECKLIMIT				\
 1:;						\
	MOVE(stdlink, temp, pc)	;		\
	CMP_L(heap_allocation_limit, heap_allocation_pointer);		\
	JB(9f);					\
	CALL(CSYM(call_heapcleaner));			\
	JMP(1b);				\
 9:

// *********************************************************************
	SEG_TEXT
	ALIGNTEXT4

// The return fate for the Mythryl signal handler.
//
LIB7_CODE_HDR( return_from_signal_handler_asm)
	MOV_L(CONST(HEAP_VOID),stdlink)
	MOV_L(CONST(HEAP_VOID),stdclos)
	MOV_L(CONST(HEAP_VOID),pc)
	MOV_L(CONST(REQUEST_RETURN_FROM_SIGNAL_HANDLER), request_w)
	JMP(CSYM(set_request))

// Here we pick up execution from where we were
// before we went off to handle a POSIX signal:
// This is a standard two-argument function, thus the closure is in fate.
//
ENTRY( resume_after_handling_signal )
	MOV_L( CONST( REQUEST_RESUME_SIGNAL_HANDLER ), request_w)
	JMP( CSYM(set_request))

// return_from_software_generated_periodic_event_handler_asm:
// The return fate for the Mythryl
// software generated periodic events handler.
//
LIB7_CODE_HDR( return_from_software_generated_periodic_event_handler_asm )
	MOV_L(CONST(REQUEST_RETURN_FROM_SOFTWARE_GENERATED_PERIODIC_EVENT_HANDLER), request_w)
	MOV_L(CONST(HEAP_VOID),stdlink)
	MOV_L(CONST(HEAP_VOID),stdclos)
	MOV_L(CONST(HEAP_VOID),pc)
	JMP(CSYM(set_request))

// Here we pick up execution from where we were
// before we went off to handle a software generated
// periodic event:
//
ENTRY( resume_after_handling_software_generated_periodic_event )
	MOV_L(CONST(REQUEST_RESUME_SOFTWARE_GENERATED_PERIODIC_EVENT_HANDLER), request_w)
	JMP( CSYM(set_request) )

// Exception handler for Mythryl functions called from C.
// We delegate uncaught-exception handling to
//     handle_uncaught_exception  in  src/c/main/runtime-exception-stuff.c
// We get invoked courtesy of being stuffed into
//     task->exception_fate
// in  src/c/main/run-mythryl-code-and-runtime-eventloop.c
// and src/c/cleaner/import-heap.c
//
LIB7_CODE_HDR(handle_uncaught_exception_closure_asm)
	MOV_L(CONST(REQUEST_HANDLE_UNCAUGHT_EXCEPTION), request_w)
	MOVE(stdlink,temp,pc)
	JMP(CSYM(set_request))


// Here to return to                                     run_mythryl_task_and_runtime_eventloop  in   src/c/main/run-mythryl-code-and-runtime-eventloop.c
// and thence to whoever called it.  If the caller was   load_and_run_heap_image                 in   src/c/main/load-and-run-heap-image.c
// this will return us to                                main                                    in   src/c/main/runtime-main.c
// which will print stats
// and exit(), but                   if the caller was   no_args_entry or some_args_entry        in   src/c/lib/ccalls/ccalls-fns.c
// then we may have some scenario
// where C calls Mythryl which calls C which ...
// and we may just be unwinding one level.
//    The latter can only happen with the
// help of the src/lib/c-glue-old/ stuff,
// which is currently non-operational.
//
// run_mythryl_task_and_runtime_eventloop is also called by
//     src/c/multicore/sgi-multicore.c
//     src/c/multicore/solaris-multicore.c
// but that stuff is also non-operational (I think) and
// we're not supposed to return to caller in those cases.
//
// We get slotted into task->fate by   save_c_state           in   src/c/main/runtime-state.c 
// and by                              run_mythryl_function   in   src/c/main/run-mythryl-code-and-runtime-eventloop.c
// and by                              import_heap_image      in   src/c/cleaner/import-heap.c
// and by                              mc_acquire_pthread     in   src/c/multicore/sgi-multicore.c
// and by                              mc_acquire_pthread     int  src/c/multicore/solaris-multicore.c
//
LIB7_CODE_HDR(return_to_c_level_asm)
	MOV_L(CONST(REQUEST_RETURN_TO_C_LEVEL), request_w)
	MOV_L(CONST(HEAP_VOID),stdlink)
	MOV_L(CONST(HEAP_VOID),stdclos)
	MOV_L(CONST(HEAP_VOID),pc)
	JMP(CSYM(set_request))



// Request a fault.  The floating point coprocessor must be reset
// (thus trashing the FP registers) since we do not know whether a 
// value has been pushed into the temporary "register".	 This is OK 
// because no floating point registers will be live at the start of 
// the exception handler.
//
ENTRY(request_fault)
	CALL(CSYM(FPEEnable))          // Does not trash any general regs.
	MOV_L(CONST(REQUEST_FAULT), request_w)
	MOVE(stdlink,temp,pc)
	JMP(CSYM(set_request))

// find_cfun : (String, String) -> Cfunction			// (library-name, function-name) -> Cfunction -- see comments in   src/c/cleaner/mythryl-callable-cfun-hashtable.c
//
// We get called (only) from:
//
//     src/lib/std/src/unsafe/mythryl-callable-c-library-interface.pkg	
//
// We pass the buck via    REQUEST_FIND_CFUN	in    src/c/main/run-mythryl-code-and-runtime-eventloop.c
// to                      get_mythryl_callable_c_function		in    src/c/lib/mythryl-callable-c-libraries.c
//
LIB7_CODE_HDR(find_cfun_asm)
	CHECKLIMIT
	MOV_L(CONST(REQUEST_FIND_CFUN), request_w)
	JMP(CSYM(set_request))

LIB7_CODE_HDR(make_package_literals_via_bytecode_interpreter_asm)
	CHECKLIMIT
	MOV_L(CONST(REQUEST_MAKE_PACKAGE_LITERALS_VIA_BYTECODE_INTERPRETER), request_w)
	JMP(CSYM(set_request))



// Invoke a C-level function (obtained from find_cfun above) from the Mythryl level.
// We get called (only) from
//
//     src/lib/std/src/unsafe/mythryl-callable-c-library-interface.pkg
//
LIB7_CODE_HDR(call_cfun_asm)					// See call_cfun in src/lib/core/init/runtime.pkg
	CHECKLIMIT
	MOV_L(CONST(REQUEST_CALL_CFUN), request_w)
	JMP(CSYM(set_request))

// This is the entry point called from Mythryl to start a heapcleaning.
//						Allen 6/5/1998
ENTRY(call_heapcleaner)
	POP_L(pc)
	MOV_L(CONST(REQUEST_CLEANING), request_w)
	//
	// FALL INTO set_request

ENTRY(set_request)
	// temp holds task_ptr, valid request in request_w.
	// Save registers:
	MOV_L( task_ptr, temp)
	MOV_L( heap_allocation_pointer, REGOFF( heap_allocation_pointer_byte_offset_in_task_struct, temp))
	MOV_L( stdarg,			REGOFF(                argument_byte_offset_in_task_struct, temp))
	MOV_L( stdfate,			REGOFF(                    fate_byte_offset_in_task_struct, temp))

#define	temp2 heap_allocation_pointer
	// Note that we have left Mythryl code:
	//
	MOV_L(REGOFF(pthread_byte_offset_in_task_struct,temp), temp2)
	MOV_L(CONST(0), REGOFF(executing_mythryl_code_byte_offset_in_pthread_struct,temp2))

	MOV_L(misc0, REGOFF(callee_saved_register_0_byte_offset_in_task_struct,temp))
	MOV_L(misc1, REGOFF(callee_saved_register_1_byte_offset_in_task_struct,temp))
	MOV_L(misc2, REGOFF(callee_saved_register_2_byte_offset_in_task_struct,temp))

	// Save vregs before the stack frame is popped:
	//
	MOVE (heap_allocation_limit,	temp2, REGOFF( heap_allocation_limit_byte_offset_in_task_struct, temp ))
	MOVE (exnfate,			temp2, REGOFF(        exception_fate_byte_offset_in_task_struct, temp )) 
	MOVE (stdclos,			temp2, REGOFF(               closure_byte_offset_in_task_struct, temp ))
	MOVE (stdlink,			temp2, REGOFF(         link_register_byte_offset_in_task_struct, temp ))
	MOVE (pc,			temp2, REGOFF(       program_counter_byte_offset_in_task_struct, temp ))
	MOVE (heap_changelog_ptr,	temp2, REGOFF(        heap_changelog_byte_offset_in_task_struct, temp ))
	MOVE (current_thread_ptr,	temp2, REGOFF(                thread_byte_offset_in_task_struct, temp ))
#undef	temp2	
	
	// Return val of function is request code:
	//
	MOV_L(request_w,creturn)

	// Pop the stack frame and return to  run_mythryl_task_and_runtime_eventloop()  in  src/c/main/run-mythryl-code-and-runtime-eventloop.c
#if defined(OPSYS_DARWIN)
	LEA_L(REGOFF(LIB7_FRAME_SIZE+12,ESP),ESP)
#else
	MOV_L(espsave, ESP)
#endif
	CALLEE_RESTORE
	RET

	SEG_TEXT
	ALIGNTEXT4
ENTRY(asm_run_mythryl_task)
	MOV_L (REGOFF(4,ESP), temp)			// Get argument (Task*)
	CALLEE_SAVE
#if defined(OPSYS_DARWIN)
        // MacOS X frames must be 16-byte aligned.
        // We have 20 bytes on the stack for the
	// return PC and callee-saves, so we need
        // a 12-byte pad:
        //
	SUB_L (CONST(LIB7_FRAME_SIZE+12), ESP)
#else
	// Align sp on 8 byte boundary.
	// We assume that the stack starts out
	// being at least word aligned, but who knows ...
	//
	MOV_L (ESP,EBX)
	OR_L  (CONST(4), ESP)		
	SUB_L (CONST(4), ESP)				// Stack grows from high to low.
	SUB_L (CONST(LIB7_FRAME_SIZE), ESP)
	MOV_L (EBX,espsave)
#endif
	
#define temp2	EBX
        // Initialize the Mythryl stackframe:
	//
	MOVE(REGOFF(           exception_fate_byte_offset_in_task_struct, temp),  temp2, exnfate)
	MOVE(REGOFF(    heap_allocation_limit_byte_offset_in_task_struct, temp),  temp2, heap_allocation_limit)
	MOVE(REGOFF(           heap_changelog_byte_offset_in_task_struct, temp),  temp2, heap_changelog_ptr)
	MOVE(REGOFF(                   thread_byte_offset_in_task_struct, temp),  temp2, current_thread_ptr)
	LEA_L(CSYM(call_heapcleaner), temp2)
	MOV_L(temp2, run_heapcleaner_ptr)
	MOV_L(temp, task_ptr)

	// vregs:
	MOVE	(REGOFF(        link_register_byte_offset_in_task_struct, temp),  temp2, stdlink)
	MOVE	(REGOFF(              closure_byte_offset_in_task_struct, temp),  temp2, stdclos)

	// PC:
	MOVE    (REGOFF(      program_counter_byte_offset_in_task_struct, temp), temp2, pc)
#undef	temp2

	// Load Mythryl registers:
	//
	MOV_L(REGOFF(             heap_allocation_pointer_byte_offset_in_task_struct, temp), heap_allocation_pointer)
	MOV_L(REGOFF(                    fate_byte_offset_in_task_struct, temp), stdfate)
	MOV_L(REGOFF(                argument_byte_offset_in_task_struct, temp), stdarg)
	MOV_L(REGOFF( callee_saved_register_0_byte_offset_in_task_struct, temp), misc0)
	MOV_L(REGOFF( callee_saved_register_1_byte_offset_in_task_struct, temp), misc1)
	MOV_L(REGOFF( callee_saved_register_2_byte_offset_in_task_struct, temp), misc2)

	MOV_L(ESP,CSYM(LIB7_intel32Frame))						// Frame ptr for signal handler.

	PUSH_L(misc2)								// Free up a register.
	PUSH_L(temp)								// Save task temporarily.

#define	tmpreg	misc2

	// Note that we are entering Mythryl:
	//
	MOV_L(REGOFF(pthread_byte_offset_in_task_struct,temp),temp)		// temp is now pthread.
#define pthread	temp
	MOV_L(CONST(1),REGOFF( executing_mythryl_code_byte_offset_in_pthread_struct, pthread ))

	// Handle signals:
	//
	MOV_L(REGOFF( all_posix_signals_seen_count_byte_offset_in_pthread_struct, pthread), tmpreg)
	CMP_L(REGOFF( all_posix_signals_done_count_byte_offset_in_pthread_struct, pthread), tmpreg)
	
#undef  tmpreg
	JNE(pending)

restore_and_jmp_lib7:
	POP_L(temp)								// Restore temp to task.
	POP_L(misc2)
	
jmp_lib7:
	CMP_L(heap_allocation_limit, heap_allocation_pointer)
	JMP(CODEPTR(REGOFF(program_counter_byte_offset_in_task_struct,temp)))	// Jump to Mythryl code.


pending:
										// Currently handling signal?

	CMP_L(CONST(0), REGOFF( mythryl_handler_for_posix_signal_is_running_byte_offset_in_pthread_struct, pthread ))   
	JNE( restore_and_jmp_lib7 )
										// Handler trap is now pending.
	movl	IMMED(1), posix_signal_pending_byte_offset_in_pthread_struct( pthread ) 

	// Must restore here because heap_allocation_limit is on stack  	// XXX
	//
	POP_L(temp)								// Restore temp to task
	POP_L(misc2)

	MOV_L(heap_allocation_pointer,heap_allocation_limit)
	JMP(jmp_lib7)								// Jump to Mythryl code.
#undef  pthread

// ----------------------------------------------------------------------
// make_typeagnostic_rw_vector : (Int, X) -> Rw_Vector(X)
// Allocate a new Rw_Vector and initialize with given value.
// This can trigger cleaning.
//
LIB7_CODE_HDR(make_typeagnostic_rw_vector_asm)
	CHECKLIMIT
	MOV_L (REGIND(stdarg), temp)						// temp := length in words.
	SAR_L (CONST(1), temp)							// temp := length untagged.
	CMP_L (CONST(MAX_AGEGROUP0_ALLOCATION_SIZE_IN_WORDS), temp)			// Is this a small chunk?
	JGE (3f)

#define temp1 misc0
#define temp2 misc1
	PUSH_L(misc0)								// Save misc0.
	PUSH_L(misc1)								// Save misc1.
	
	MOV_L (temp, temp1)
	SAL_L (CONST(TAGWORD_LENGTH_FIELD_SHIFT),temp1)				// Build tagword in temp1.
	OR_L (CONST(MAKE_BTAG(RW_VECTOR_DATA_BTAG)),temp1)
	MOV_L (temp1,REGIND(heap_allocation_pointer))				// Store tagword.
	ADD_L (CONST(4),heap_allocation_pointer)				// heap_allocation_pointer++
	MOV_L (heap_allocation_pointer, temp1)					// temp1 := array data ptr
	MOV_L (REGOFF(4,stdarg), temp2)						// temp2 := initial value
2:	
	MOV_L (temp2, REGIND(heap_allocation_pointer))				// Initialize array.
	ADD_L (CONST(4), heap_allocation_pointer)
	SUB_L (CONST(1), temp)
	JNE (2b)

	// Allocate Rw_Vector header:
	//
	MOV_L (CONST(TYPEAGNOSTIC_RW_VECTOR_TAGWORD), REGIND(heap_allocation_pointer))				// Tagword in temp.
	ADD_L (CONST(4), heap_allocation_pointer)						// heap_allocation_pointer++
	MOV_L (REGIND(stdarg), temp)						// temp := length
	MOV_L (heap_allocation_pointer, stdarg)						// result = header addr
	MOV_L (temp1, REGIND(heap_allocation_pointer))						// Store pointer to data.
	MOV_L (temp, REGOFF(4,heap_allocation_pointer))					// Store length.
	ADD_L (CONST(8), heap_allocation_pointer)

	POP_L (misc1)
	POP_L (misc0)
	CONTINUE
#undef  temp1
#undef  temp2
3:
	MOV_L(CONST(REQUEST_MAKE_TYPEAGNOSTIC_RW_VECTOR), request_w)
	MOVE	(stdlink, temp, pc)
	JMP(CSYM(set_request))
	

// make_float64_rw_vector : Int -> Float64_Rw_Vector
//
LIB7_CODE_HDR(make_float64_rw_vector_asm)
	CHECKLIMIT
#define temp1 misc0
        PUSH_L(misc0)						// Free temp1.
	MOV_L(stdarg,temp)					// temp := length
	SAR_L(CONST(1),temp)					// temp := untagged length
	SHL_L(CONST(1),temp)					// temp := length in words
	CMP_L(CONST(MAX_AGEGROUP0_ALLOCATION_SIZE_IN_WORDS),temp)
	JGE(2f)

	OR_L(CONST(4),heap_allocation_pointer)			// Align heap_allocation_pointer.	// 64-bit issue	;  this aligns for 32-bit tagword followed by 64-bit float value; unecessary on 64-bit system.

	// Allocate the data chunk:
	//
	MOV_L(temp, temp1)
	SHL_L(CONST(TAGWORD_LENGTH_FIELD_SHIFT),temp1)  			// temp1 := tagword
	OR_L(CONST(MAKE_BTAG(EIGHT_BYTE_ALIGNED_NONPOINTER_DATA_BTAG)),temp1)
	MOV_L(temp1,REGIND(heap_allocation_pointer))				// Store tagword
	ADD_L(CONST(4), heap_allocation_pointer)				// heap_allocation_pointer++
	MOV_L(heap_allocation_pointer, temp1)					// temp1 := data chunk
	SHL_L(CONST(2),temp)					// temp := length in bytes
	ADD_L(temp, heap_allocation_pointer)					// heap_allocation_pointer += length

	// Allocate the header chunk:
	//
	MOV_L(CONST(FLOAT64_RW_VECTOR_TAGWORD),REGIND(heap_allocation_pointer))		// Header tagword.
	ADD_L(CONST(4), heap_allocation_pointer)				// heap_allocation_pointer++
	MOV_L(temp1, REGIND(heap_allocation_pointer))				// header data field
	MOV_L(stdarg, REGOFF(4,heap_allocation_pointer))			// header length field
	MOV_L(heap_allocation_pointer, stdarg)					// stdarg := header chunk
	ADD_L(CONST(8), heap_allocation_pointer)				// heap_allocation_pointer += 2

	POP_L(misc0)						// Restore temp1.
	CONTINUE
2:
	POP_L(misc0)						// Restore temp1.
	MOV_L(CONST(REQUEST_ALLOCATE_FLOAT64_VECTOR), request_w)
	MOVE	(stdlink, temp, pc)
	JMP(CSYM(set_request))
#undef temp1


// make_unt8_rw_vector : Int -> Unt8_Rw_Vector
//
LIB7_CODE_HDR(make_unt8_rw_vector_asm)
	CHECKLIMIT
	MOV_L(stdarg,temp)					// temp := length(tagged int)
	SAR_L(CONST(1),temp)					// temp := length(untagged)
	ADD_L(CONST(3),temp)
	SAR_L(CONST(2),temp)					// temp := length(words)
	CMP_L(CONST(MAX_AGEGROUP0_ALLOCATION_SIZE_IN_WORDS),temp)	// small chunk?
	JMP(2f)
	JGE(2f)							// XXXXX

#define	temp1	misc0
	PUSH_L(misc0)

	// Allocate the data chunk:
	//
	MOV_L(temp, temp1)					// temp1 :=  tagword.
	SHL_L(CONST(TAGWORD_LENGTH_FIELD_SHIFT),temp1)
	OR_L(CONST(MAKE_BTAG(FOUR_BYTE_ALIGNED_NONPOINTER_DATA_BTAG)),temp1)
	MOV_L(temp1, REGIND(heap_allocation_pointer))				// Store tagword.
	ADD_L(CONST(4), heap_allocation_pointer)				// heap_allocation_pointer++
	MOV_L(heap_allocation_pointer, temp1)					// temp1 := data chunk
	SHL_L(CONST(2), temp)					// temp := length in bytes
	ADD_L(temp, heap_allocation_pointer)					// heap_allocation_pointer += length

	// Allocate the header chunk:
	//
	MOV_L(CONST(UNT8_RW_VECTOR_TAGWORD), REGIND(heap_allocation_pointer))		// Header tagword.
	ADD_L(CONST(4),heap_allocation_pointer)				// heap_allocation_pointer++
	MOV_L(temp1, REGIND(heap_allocation_pointer))				// header data field
	MOV_L(stdarg, REGOFF(4,heap_allocation_pointer))			// header length field
	MOV_L(heap_allocation_pointer, stdarg)					// stdarg := header chunk
	ADD_L(CONST(8),heap_allocation_pointer)				// heap_allocation_pointer := 2
	POP_L(misc0)
	CONTINUE
#undef  temp1
2:
	MOV_L(CONST(REQUEST_ALLOCATE_BYTE_VECTOR), request_w)
	MOVE	(stdlink, temp, pc)
	JMP(CSYM(set_request))


// make_string : Int -> String
//
LIB7_CODE_HDR(make_string_asm)
	CHECKLIMIT
	MOV_L(stdarg,temp)
	SAR_L(CONST(1),temp)					// temp := length(untagged)
	ADD_L(CONST(4),temp)		
	SAR_L(CONST(2),temp)					// temp := length(words)
	CMP_L(CONST(MAX_AGEGROUP0_ALLOCATION_SIZE_IN_WORDS),temp)
	JGE(2f)

	PUSH_L(misc0)						// Free misc0.
#define	temp1	misc0

	MOV_L(temp, temp1)
	SHL_L(CONST(TAGWORD_LENGTH_FIELD_SHIFT),temp1)				// Build tagword in temp1.
	OR_L(CONST(MAKE_BTAG(FOUR_BYTE_ALIGNED_NONPOINTER_DATA_BTAG)), temp1)
	MOV_L(temp1, REGIND(heap_allocation_pointer))				// Store the data pointer.
	ADD_L(CONST(4),heap_allocation_pointer)				// heap_allocation_pointer++

	MOV_L(heap_allocation_pointer, temp1)					// temp1 := data chunk
	SHL_L(CONST(2),temp)					// temp := length in bytes
	ADD_L(temp, heap_allocation_pointer)					// heap_allocation_pointer += length
	MOV_L(CONST(0),REGOFF(-4,heap_allocation_pointer))			// Zero out the last word.

	// Allocate the header chunk
	//
	MOV_L(CONST(STRING_TAGWORD), temp)				// Header tagword.
	MOV_L(temp, REGIND(heap_allocation_pointer))
	ADD_L(CONST(4), heap_allocation_pointer)				// heap_allocation_pointer++
	MOV_L(temp1, REGIND(heap_allocation_pointer))				// Header data field.
	MOV_L(stdarg, REGOFF(4,heap_allocation_pointer))			// Header length field.
	MOV_L(heap_allocation_pointer, stdarg)					// stdarg := header chunk
	ADD_L(CONST(8), heap_allocation_pointer)		
	
	POP_L(misc0)						// Restore misc0.
#undef  temp1
	CONTINUE
2:
	MOV_L(CONST(REQUEST_ALLOCATE_STRING), request_w)
	MOVE	(stdlink, temp, pc)
	JMP(CSYM(set_request))

// make_vector_asm:  (Int, List(X)) -> Vector(X)			// (length_in_slots, initializer_list) -> result_vector
//
//	Create a vector and initialize from given list.
//
//	Caller guarantees that length_in_slots is
//      positive and also the length of the list.
//	For a sample client call see
//          fun vector
//	in
//	    src/lib/core/init/pervasive.pkg
//
LIB7_CODE_HDR(make_vector_asm)
	CHECKLIMIT
	PUSH_L(misc0)
	PUSH_L(misc1)
#define	temp1	misc0
#define temp2   misc1	
	MOV_L( REGIND(stdarg), temp)					// temp := length(tagged)
	MOV_L( temp, temp1)
	SAR_L( CONST(1), temp1)						// temp1 := length(untagged)
	CMP_L( CONST( MAX_AGEGROUP0_ALLOCATION_SIZE_IN_WORDS ), temp1)	// Is this a small chunk (i.e., one allowed in the arena)?
	JGE(3f)								// branch if so.

	// Allocate data chunk:
	//	
	SHL_L( CONST( TAGWORD_LENGTH_FIELD_SHIFT ), temp1)		// Build tagword in temp1.
	OR_L(  CONST( MAKE_BTAG( RO_VECTOR_DATA_BTAG )), temp1)
	MOV_L( temp1, REGIND( heap_allocation_pointer ))				// Store tagword.
	ADD_L( CONST(4), heap_allocation_pointer)					// heap_allocation_pointer++
	MOV_L( REGOFF(4, stdarg), temp1)				// temp1 := list
	MOV_L( heap_allocation_pointer, stdarg)					// stdarg := vector

2:
	MOV_L( REGIND(temp1), temp2)					// temp2 := head(temp1)
	MOV_L( temp2, REGIND( heap_allocation_pointer ))				// Store word in vector.
	ADD_L( CONST(4), heap_allocation_pointer)					// heap_allocation_pointer++
	MOV_L( REGOFF(4,temp1), temp1)					// temp1 := tail(temp1)
	CMP_L( CONST(HEAP_NIL), temp1)					// temp1 = NIL?
	JNE(2b)								// Loop if not.

	// Allocate header chunk:
	//	
	MOV_L( CONST( TYPEAGNOSTIC_RO_VECTOR_TAGWORD ), temp1)		// Tagword in temp1.
	MOV_L( temp1, REGIND(heap_allocation_pointer))					// Store tagword.
	ADD_L( CONST(4), heap_allocation_pointer)					// heap_allocation_pointer++
	MOV_L( stdarg, REGIND(heap_allocation_pointer))					// Header data field.
	MOV_L( temp, REGOFF(4,heap_allocation_pointer))					// Header length.
	MOV_L( heap_allocation_pointer, stdarg)						// result = header chunk
	ADD_L( CONST(8), heap_allocation_pointer)					// heap_allocation_pointer += 2

	POP_L( misc1 )
	POP_L( misc0 )
	CONTINUE
3:
	POP_L(misc1)
	POP_L(misc0)
	MOV_L(CONST(REQUEST_MAKE_TYPEAGNOSTIC_RO_VECTOR), request_w)
	MOVE	(stdlink, temp, pc)
	JMP(CSYM(set_request))
#undef  temp1
#undef  temp2	
	
// try_lock: Spin_Lock -> Bool. 
// low-level test-and-set style primitive for mutual-exclusion among 
// processors.	For now, we only provide a uni-processor trivial version.
//
LIB7_CODE_HDR(try_lock_asm)
#if (MAX_PROCS > 1)
#  error multiple processors not supported
#else // (MAX_PROCS == 1)
	MOV_L(REGIND(stdarg), temp)				// Get old value of lock.
	MOV_L(CONST(1), REGIND(stdarg))				// Set the lock to HEAP_FALSE.
	MOV_L(temp, stdarg)					// Return old value of lock.
	CONTINUE
#endif

// unlock :  Release a spinlock.
//
LIB7_CODE_HDR(unlock_asm)
#if (MAX_PROCS > 1)
    #error multiple processors not supported
#else // (MAX_PROCS == 1)
	MOV_L(CONST(3), REGIND(stdarg))				// Store HEAP_TRUE into lock.
	MOV_L(CONST(1), stdarg)					// Return Void.
	CONTINUE
#endif


// ******************** Floating point functions. ********************

#define FPOP	fstp %st	// Pop the floating point register stack.


// Temporary storage for the old and new floating point control
// word.  We don't use the stack to for this, since doing so would 
// change the offsets of the pseudo-registers.
	DATA
	ALIGN4
old_controlwd:	
	.word	0
new_controlwd:	
	.word	0
	TEXT
	ALIGN4


// Initialize the 80387 floating point coprocessor.  First, the floating
// point control word is initialized (undefined fields are left
// unchanged).	Rounding control is set to "nearest" (although floor_a
// needs "toward negative infinity").  Precision control is set to
// "double".  The precision, underflow, denormal 
// overflow, zero divide, and invalid operation exceptions
// are masked.  Next, seven of the eight available entries on the
// floating point register stack are claimed (see intel32/intel32.pkg).
//
// NB: This cannot trash any registers because it's called from request_fault.
//
ENTRY(FPEEnable)
	FINIT
	SUB_L(CONST(4), ESP)			// Temp space.	Keep stack aligned.
	FSTCW(REGIND(ESP))			// Store FP control word.
						// Keep undefined fields, clear others.
	AND_W(CONST(0xf0c0), REGIND(ESP))
	OR_W(CONST(0x023f), REGIND(ESP))	// Set fields (see above).
	FLDCW(REGIND(ESP))			// Install new control word.
	ADD_L(CONST(4), ESP)
	RET

#if (defined(OPSYS_LINUX) || defined(OPSYS_CYGWIN) || defined(OPSYS_SOLARIS))
ENTRY(fegetround)
	SUB_L(CONST(4), ESP)			// Allocate temporary space.
	FSTCW(REGIND(ESP))			// Store fp control word.
	SAR_L(CONST(10),REGIND(ESP))		// Rounding mode is at bit 10 and 11.
	AND_L(CONST(3), REGIND(ESP))		// Mask two bits.
	MOV_L(REGIND(ESP),EAX)			// Return rounding mode.
	ADD_L(CONST(4), ESP)			// Deallocate space.
	RET
  	
ENTRY(fesetround)
	SUB_L(CONST(4), ESP)			// Allocate temporary space
	FSTCW(REGIND(ESP))			// Store fp control word.
	AND_W(CONST(0xf3ff), REGIND(ESP))	// Clear rounding field.
	MOV_L(REGOFF(8,ESP), EAX)		// New rounding mode.
	SAL_L(CONST(10), EAX)			// Move to right place.
	OR_L(EAX,REGIND(ESP))			// New control word.
	FLDCW(REGIND(ESP))			// Load new control word.
	ADD_L(CONST(4), ESP)			// Deallocate space.
	RET
#endif


// floor : real -> int
// Return the nearest integer that is less or equal to the argument.
//	 Caller's responsibility to make sure arg is in range.

LIB7_CODE_HDR(floor_asm)
	FSTCW(old_controlwd)			// Get FP control word.
	MOV_W(old_controlwd, AX)
	AND_W(CONST(0xf3ff), AX)		// Clear rounding field.
	OR_W(CONST(0x0400), AX)			// Round towards neg. infinity.
	MOV_W(AX, new_controlwd)
	FLDCW(new_controlwd)			// Install new control word.

	FLD_D(REGIND(stdarg))
	SUB_L(CONST(4), ESP)
	FISTP_L(REGIND(ESP))			// Round, store, and pop.
	POP_L(stdarg)
	SAL_L(CONST(1), stdarg)			// Tag the resulting integer.
	INC_L(stdarg)

	FLDCW(old_controlwd)			// Restore old FP control word.
	CONTINUE

// logb : real -> int
// Extract the unbiased exponent pointed to by stdarg.
// Note: Using fxtract, and fistl does not work for inf's and nan's.
//
LIB7_CODE_HDR(logb_asm)
	MOV_L(REGOFF(4,stdarg),temp)		// msb for little endian arch
	SAR_L(CONST(20), temp)			// throw out 20 bits
	AND_L(CONST(0x7ff),temp)		// clear all but 11 low bits
	SUB_L(CONST(1023), temp)		// unbias
	SAL_L(CONST(1), temp)			// room for tag bit
	ADD_L(CONST(1), temp)			// tag bit
	MOV_L(temp, stdarg)
	CONTINUE
	

// scalb : (real * int) -> real
// Scale the first argument by 2 raised to the second argument.	 Raise
// Float("underflow") or Float("overflow") as appropriate.
// NB: We assume the first floating point "register" is
// caller-save, so we can use it here (see intel32/intel32.pkg).

LIB7_CODE_HDR(scalb_asm)
	CHECKLIMIT
	PUSH_L(REGOFF(4,stdarg))				// Get copy of scalar.				64-bit issue
	SAR_L(CONST(1), REGIND(ESP))				// Untag it.
	FILD_L(REGIND(ESP))					// Load it ...
//	fstp	%st(1)						// ... into 1st FP reg.
	MOV_L(REGIND(stdarg), temp)				// Get pointer to real.
	FLD_D(REGIND(temp))					// Load it into temp.

	FSCALE							// Multiply exponent by scalar.
	MOV_L(CONST(FLOAT64_TAGWORD), REGIND(heap_allocation_pointer))
	FSTP_D(REGOFF(4,heap_allocation_pointer))		// Store resulting float.			64-bit issue
	ADD_L(CONST(4), heap_allocation_pointer)		// Allocate word for tag.			64-bit issue
	MOV_L(heap_allocation_pointer, stdarg)			// Return a pointer to the float.
	ADD_L(CONST(8), heap_allocation_pointer)		// Allocate room for float.
	FSTP_D(REGIND(ESP))			
	ADD_L(CONST(4), ESP)					// Discard copy of scalar.			64-bit issue
	CONTINUE



// COPYRIGHT (c) 1995 AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

