// libmythryl-heap.c

// This file defines the "heap" library of Mythryl-callable
// C functions, accessible at the Mythryl level via:
//
//     my  get_program_name:  Void -> String
//         =
//	   mythryl_callable_c_library_interface::find_c_function { lib_name => "heap", fun_name => "program_name_from_commandline" };
// 
// or such.
//
// The functions we export here get used in
//
//     src/lib/compiler/execution/code-segments/code-segment.pkg
//     src/lib/src/lib/thread-kit/src/core-thread-kit/threadkit-debug.pkg
//     src/lib/src/lib/thread-kit/src/glue/thread-scheduler-control-g.pkg
//     src/lib/src/lib/thread-kit/src/glue/threadkit-export-function-g.pkg
//     src/lib/std/commandline.pkg
//     src/lib/std/src/nj/export.pkg
//     src/lib/std/src/nj/heap-debug.pkg
//     src/lib/std/src/nj/heapcleaner-control.pkg
//     src/lib/std/src/nj/platform-properties.pkg
//     src/lib/std/src/nj/runtime-signals-guts.pkg
//     src/lib/std/src/unsafe/unsafe-chunk.pkg
//     src/lib/std/src/unsafe/unsafe.pkg

#include "../../mythryl-config.h"

#include <stdio.h>
#include <string.h>

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#if HAVE_SYS_TIME_H
#  include <sys/time.h>
#endif

#include "runtime-base.h"
#include "runtime-values.h"

#include "architecture-and-os-names-system-dependent.h"

#include "flush-instruction-cache-system-dependent.h"
#include "get-multipage-ram-region-from-os.h"

#include "heap.h"
#include "heapcleaner.h"
#include "heap-io.h"

#include "lib7-c.h"
#include "mythryl-callable-c-libraries.h"
#include "make-strings-and-vectors-etc.h"

#if defined(OPSYS_UNIX)
    #include "system-dependent-unix-stuff.h"  		// For OS_NAME.
#elif defined(OPSYS_WIN32)
    #define OS_NAME "Win32"
#endif


#define SAME_STRING(s1, s2)	(strcmp((s1), (s2)) == 0)

#define FALSE_VALUE	"FALSE"
#define TRUE_VALUE	"TRUE"

Bool do_debug_logging = FALSE;							// Used to control special debug logging.

//
//
static Val   do_allocate_codechunk   (Task* task,  Val arg) {
    //       =====================
    //
    // Mythryl type:   Int -> rw_vector_of_one_byte_unts::Rw_Vector
    //
    // Allocate a code chunk of the given size-in-bytes.
    //
    // Note: Generating the name string within the code chunk
    //       is the code generator's responsibility.
    //
    // This fn gets bound to 'alloc_code' in:
    //
    //     src/lib/compiler/execution/code-segments/code-segment.pkg

									    ENTER_MYTHRYL_CALLABLE_C_FN("do_allocate_codechunk");

    int   nbytes =   TAGGED_INT_TO_C_INT( arg );
    Val	  code   =   allocate_nonempty_code_chunk( task, nbytes );		// allocate_nonempty_code_chunk		def in    src/c/heapcleaner/make-strings-and-vectors-etc.c

    Val	               result;
    SEQHDR_ALLOC(task, result, UNT8_RW_VECTOR_TAGWORD, code, nbytes);
    return             result;
}

//
static Val   do_check_agegroup0_overrun_tripwire_buffer  (Task* task,  Val arg)   {
    //       ==========================================
    //
    // Mythryl type:   String -> Void
    //
    // This fn gets bound as   check_agegroup0_overrun_tripwire_buffer   in:
    //
    //     src/lib/std/src/nj/heap-debug.pkg
    //

									    ENTER_MYTHRYL_CALLABLE_C_FN("do_check_agegroup0_overrun_tripwire_buffer");

    char* caller = HEAP_STRING_AS_C_STRING(arg);
    //
    check_agegroup0_overrun_tripwire_buffer( task, caller );
    //
    return HEAP_VOID;
}

//
static Val   do_enable_debug_logging  (Task* task,  Val arg)   {
    //       =======================
    //
    // Mythryl type:   Void -> Void
    //
    // This fn gets bound as   enable_debug_logging   in:
    //
    //     src/lib/std/src/nj/heap-debug.pkg
    //

									    ENTER_MYTHRYL_CALLABLE_C_FN("do_enable_debug_logging");

    do_debug_logging = TRUE;
    //
    return HEAP_VOID;
}

//
static Val   do_disable_debug_logging  (Task* task,  Val arg)   {
    //       ========================
    //
    // Mythryl type:   Void -> Void
    //
    // This fn gets bound as   disable_debug_logging   in:
    //
    //     src/lib/std/src/nj/heap-debug.pkg
    //

									    ENTER_MYTHRYL_CALLABLE_C_FN("do_disable_debug_logging");

    do_debug_logging = FALSE;
    //
    return HEAP_VOID;
}

//
static Val   do_get_commandline_args   (Task* task,  Val arg)   {
    //       =======================
    //
    // Mythryl type:  Void -> List(String)
    //
    // This fn gets bound to 'get_arguments' in:
    //
    //     src/lib/std/commandline.pkg
    //

									    ENTER_MYTHRYL_CALLABLE_C_FN("do_get_commandline_args");

    return make_ascii_strings_from_vector_of_c_strings (task, commandline_arguments);
}
//
static Val   do_concatenate_two_tuples   (Task* task,  Val arg)   {
    //       =========================
    //
    // Mythryl type:   (Chunk, Chunk) -> Chunk
    //
    // This fn gets bound as   r_meld   in:
    //
    //     src/lib/std/src/unsafe/unsafe-chunk.pkg

									    ENTER_MYTHRYL_CALLABLE_C_FN("do_concatenate_two_tuples");

    Val    r1 = GET_TUPLE_SLOT_AS_VAL(arg,0);
    Val    r2 = GET_TUPLE_SLOT_AS_VAL(arg,1);

    if (r1 == HEAP_VOID)	return r2;
    else if (r2 == HEAP_VOID)	return r1;
    else {
      Val  result =   concatenate_two_tuples (task, r1, r2);					// concatenate_two_tuples	def in   src/c/heapcleaner/tuple-ops.c

	if (result == HEAP_VOID)   return RAISE_ERROR( task, "recordmeld: not a record");
	else                       return result;
    }
}
//
static Val   do_debug   (Task* task,  Val arg)   {
    //       ========
    //
    // Mythryl type:   String -> Void
    //
    // This fn gets bound to 'debug'     in:   src/lib/std/src/nj/runtime-signals-guts.pkg
    // This fn gets bound to 'say_debug' in:   src/lib/src/lib/thread-kit/src/core-thread-kit/threadkit-debug.pkg
    //     

									    ENTER_MYTHRYL_CALLABLE_C_FN("do_debug");

    char* heap_string = HEAP_STRING_AS_C_STRING(arg);

    Mythryl_Heap_Value_Buffer string_buf;

    {	char* c_string = buffer_mythryl_heap_value( &string_buf, (void*) heap_string, strlen( heap_string ) +1 );	// '+1' for terminal NUL at end of string.

	RELEASE_MYTHRYL_HEAP( task->pthread, "do_debug", arg );
	    //
	    debug_say( c_string );					// debug_say	is from   src/c/main/error-reporting.c
	    //
	RECOVER_MYTHRYL_HEAP( task->pthread, "do_debug" );

	unbuffer_mythryl_heap_value( &string_buf );
    }

    return HEAP_VOID;
}
//
static Val   do_dummy   (Task* task,  Val arg)   {
    //       ========
    // 
    // Mythryl type:  String -> Void
    //
    // The string argument can be used as a unique marker.
    //

									    ENTER_MYTHRYL_CALLABLE_C_FN("do_dummy");


    /*
      char	*s = HEAP_STRING_AS_C_STRING(arg);
    */

    return HEAP_VOID;
}
//
static Val   do_spawn_to_disk   (Task* task,  Val arg)   {    // : 
    //       ================
    //
    // Mythryl type:  (String, ((String, List(String)) -> Int)) -> Void
    // or maybe:      (String, (List(String) -> Void)) -> Void              XXX BUGGO FIXME figure out which, then if needed also update   src/c/lib/heap/cfun-list.h
    //
    // Save the current heap in a diskfile, tweaked
    // to begin execution with the given function
    // when next loaded and executed.
    //
    // This fn get bound to   spawn_to_disk'   in:
    //
    //     src/lib/std/src/nj/export.pkg
    //

									    ENTER_MYTHRYL_CALLABLE_C_FN("do_spawn_to_disk");

    char	cwd[      1024 ];
    char	filename[ 1024 ];

    Val	lib7_filename = GET_TUPLE_SLOT_AS_VAL( arg, 0 );
    Val	funct          = GET_TUPLE_SLOT_AS_VAL( arg, 1 );

    FILE*	file;
    int		status;

    if (!getcwd( cwd, 1024 )) { strcpy( cwd, "." ); }

    strcpy( filename, HEAP_STRING_AS_C_STRING(lib7_filename) );

    fprintf(
        stderr,
        "\n                            export-fun.c:   Writing   executable (heap image) %s/%s\n\n",
        cwd,
        filename
    );

    if (!(file = fopen(filename, "wb"))) {
      return RAISE_ERROR( task, "Unable to open file for writing");
    }

    status =  export_fn_image( task, funct, file );				// export_fn_image	def in   src/c/heapcleaner/export-heap.c

    fclose (file);

    if (status == SUCCESS) 	print_stats_and_exit( 0 );
    else	                die( "Export-fn call failed" );

    // NB: It would be nice to raise a RUNTIME_EXCEPTION exception here,
    // but the Mythryl state has been trashed as a side-effect of the
    // export operation.
    //	    return RAISE_ERROR(task, "export failed");

    exit(1);			// Cannot execute; just here to prevent a compiler warning.
}

//
static Val   do_export_heap   (Task* task,  Val arg)   {
    //       ==============
    //
    // Mythryl type: String -> Bool
    //
    // Export the world to the given file and return FALSE.
    // The exported version returns TRUE when restarted.
    //
    // This fn gets bound to   export_heap   in:
    //
    //     src/lib/std/src/nj/export.pkg

									    ENTER_MYTHRYL_CALLABLE_C_FN("do_export_heap");

    char  fname[ 1024 ];
    FILE* file;

    strcpy(fname, HEAP_STRING_AS_C_STRING(arg)); // XXX BUGGO FIXME no buffer overflow check!

    fprintf(stderr,"\n");
    fprintf(stderr,"------------------------------------------------------------------------------------------------------\n");
    fprintf(stderr," export-heap.c:do_export_heap:   Writing file '%s'\n",fname);
    fprintf(stderr,"------------------------------------------------------------------------------------------------------\n");

    if (!(file = fopen(fname, "wb"))) {
        //
        return RAISE_ERROR(task, "unable to open file for writing");
    }

    task->argument = HEAP_TRUE;

    int status =   export_heap_image( task, file );					// export_heap_image		def in    src/c/heapcleaner/export-heap.c

    fclose (file);

    if (status == SUCCESS)   return HEAP_FALSE;
    else                     return RAISE_ERROR( task, "export failed");
}

//
static Val   do_get_platform_property   (Task* task,  Val arg)   {
    //       ========================
    //
    //
    //
    //
    // Mythryl type:   String -> Null_Or(String)
    //
    //
    //
    // Current queries:
    //
    //     "OS_NAME"     -> "Linux"/"BSD"/"Cygwin" /"SunOS"/"Solaris"/"Irix"/"OSF/1"/"AIX"/"Darwin"/"HPUX"
    //     "OS_VERSION"  -> "<unknown>"
    //
    //     "HOST_ARCH"   -> "INTEL32"/"PWRPC32"/"SPARC32"/"<unknown>"
    //     "TARGET_ARCH" -> "INTEL32"/"PWRPC32"/"SPARC32"/"<unknown>"
    //
    //     "HAS_SOFTWARE_GENERATED_PERIODIC_EVENTS" -> "YES" / "NO"
    //     "HAS_MP"                                 -> "YES" / "NO"
    //
    //
    //
    // Returns:   THE <string>   for a valid query;
    //	          NULL           for an invalid one.
    //
    //
    // This fn gets bound as   get_platform_property   in:
    //
    //     src/lib/std/src/nj/platform-properties.pkg

									    ENTER_MYTHRYL_CALLABLE_C_FN("do_get_platform_property");

    char* name = HEAP_STRING_AS_C_STRING(arg);

    Val	  result;

    if (SAME_STRING("OS_NAME", name))
	result = make_ascii_string_from_c_string(task, OS_NAME);
    else if (SAME_STRING("OS_VERSION", name))
	result = make_ascii_string_from_c_string(task, "<unknown>");
    else if (SAME_STRING("HOST_ARCH", name))
#if defined(HOST_PWRPC32)
	result = make_ascii_string_from_c_string(task, "PWRPC32");
#elif defined(HOST_SPARC32)
	result = make_ascii_string_from_c_string(task, "SPARC32");
#elif defined(HOST_INTEL32)
	result = make_ascii_string_from_c_string(task, "INTEL32");
#else
	result = make_ascii_string_from_c_string(task, "<unknown>");
#endif
    else if (SAME_STRING("TARGET_ARCH", name))
#if defined(TARGET_PWRPC32)
	result = make_ascii_string_from_c_string(task, "PWRPC32");
#elif defined(TARGET_SPARC32)
	result = make_ascii_string_from_c_string(task, "SPARC32");
#elif defined(TARGET_INTEL32)
	result = make_ascii_string_from_c_string(task, "INTEL32");
#else
	result = make_ascii_string_from_c_string(task, "<unknown>");
#endif
    else if (SAME_STRING("HAS_SOFTWARE_GENERATED_PERIODIC_EVENTS", name))
#if NEED_SOFTWARE_GENERATED_PERIODIC_EVENTS
	result = make_ascii_string_from_c_string(task, TRUE_VALUE);
#else
	result = make_ascii_string_from_c_string(task, FALSE_VALUE);
#endif
    else if (SAME_STRING("HAS_MP", name))

#if NEED_PTHREAD_SUPPORT
	result = make_ascii_string_from_c_string(task, TRUE_VALUE);
#else
	result = make_ascii_string_from_c_string(task, FALSE_VALUE);
#endif
    else
	return OPTION_NULL;

    OPTION_THE(task, result, result);

    return result;
}

//
static Val   do_interval_tick__unimplemented   (Task* task,  Val arg)   {
    //       ===============================
    //
    // Mythryl type:  Void -> (Int, Int)
    //
    // This fn gets bound as   tick'   in:
    //
    //     src/lib/std/src/nj/set-sigalrm-frequency.pkg

									    ENTER_MYTHRYL_CALLABLE_C_FN("do_interval_tick__unimplemented");

    return RAISE_ERROR( task, "interval_tick unimplemented");
}



//
static Val   do_dump_gen0   (Task* task,  Val arg)   {
    //       ============
    //
    // Mythryl type:  String -> Void
    //
    // This fn gets bound as   dump_gen0   in:
    //
    //     src/lib/std/src/nj/heap-debug.pkg
    //

									    ENTER_MYTHRYL_CALLABLE_C_FN("do_dump_gen0");

    char* caller = HEAP_STRING_AS_C_STRING(arg);					// Name of calling fn; used only for human diagnostic purposes.
    //
    dump_gen0( task, caller );								// dump_gen0			is from   src/c/heapcleaner/heap-debug-stuff.c
    //
    return HEAP_VOID;
}



//
static Val   do_dump_gens   (Task* task,  Val arg)   {
    //       ============
    //
    // Mythryl type:  String -> Void
    //
    // This fn gets bound as   dump_gens   in:
    //
    //     src/lib/std/src/nj/heap-debug.pkg
    //

									    ENTER_MYTHRYL_CALLABLE_C_FN("do_dump_gens");

    char* caller = HEAP_STRING_AS_C_STRING(arg);					// Name of calling fn; used only for human diagnostic purposes.
    //
    dump_gens( task, caller );								// dump_gens			is from   src/c/heapcleaner/heap-debug-stuff.c
    //
    return HEAP_VOID;
}



//
static Val   do_dump_hugechunk_stuff   (Task* task,  Val arg)   {
    //       =======================
    //
    // Mythryl type:  String -> Void
    //
    // This fn gets bound as   dump_huge   in:
    //
    //     src/lib/std/src/nj/heap-debug.pkg
    //

									    ENTER_MYTHRYL_CALLABLE_C_FN("do_dump_hugechunk_stuff");

    char* caller = HEAP_STRING_AS_C_STRING(arg);					// Name of calling fn; used only for human diagnostic purposes.
    //
    dump_hugechunk_stuff( task, caller );						// dump_hugechunk_stuff		is from   src/c/heapcleaner/heap-debug-stuff.c
    //
    return HEAP_VOID;
}



//
static Val   do_dump_task   (Task* task,  Val arg)   {
    //       ============
    //
    // Mythryl type:  String -> Void
    //
    // This fn gets bound as   dump_task   in:
    //
    //     src/lib/std/src/nj/heap-debug.pkg
    //

									    ENTER_MYTHRYL_CALLABLE_C_FN("do_dump_task");

    char* caller = HEAP_STRING_AS_C_STRING(arg);					// Name of calling fn; used only for human diagnostic purposes.
    //
    dump_task( task, caller );								// dump_task		is from   src/c/heapcleaner/heap-debug-stuff.c
    //
    return HEAP_VOID;
}



//
static Val   do_dump_whatever   (Task* task,  Val arg)   {
    //       ================
    //
    // Mythryl type:  String -> Void
    //
    // This fn gets bound as   dump_whatever   in:
    //
    //     src/lib/std/src/nj/heap-debug.pkg
    //

									    ENTER_MYTHRYL_CALLABLE_C_FN("do_dump_whatever");

    char* caller = HEAP_STRING_AS_C_STRING(arg);					// Name of calling fn; used only for human diagnostic purposes.
    //
    dump_whatever( task, caller );							// dump_whatever	is from   src/c/heapcleaner/heap-debug-stuff.c
    //
    return HEAP_VOID;
}



//
static Val   do_make_codechunk_executable   (Task* task,  Val arg)   {
    //       ============================
    //
    // Mythryl type:  (rw_vector_of_one_byte_unts::Rw_Vector, Int) -> (Chunk -> Chunk)	// The Int is the entrypoint offset within the bytevector of executable machine code -- currently always zero in practice.
    //
    // Turn a previously constructed machine-code bytvector into a closure.
    // This requires that we flush the I-cache. (This is a no-op on intel32.)
    //
    // This fn gets bound as   make_executable   in:
    //
    //     src/lib/compiler/execution/code-segments/code-segment.pkg

									    ENTER_MYTHRYL_CALLABLE_C_FN("do_make_codechunk_executable");

    Val   seq        =  GET_TUPLE_SLOT_AS_VAL( arg, 0 );
    int   entrypoint =  GET_TUPLE_SLOT_AS_INT( arg, 1 );			// In practice entrypoint is currently always zero.

    char* code =  GET_VECTOR_DATACHUNK_AS( char*, seq );

    Val_Sized_Unt nbytes		/* This variable is unused on some platforms, so suppress 'unused var' compiler warning: */   __attribute__((unused))
        =
        GET_VECTOR_LENGTH( seq );

    flush_instruction_cache( code, nbytes );					// flush_instruction_cache is a no-op on intel32
										// flush_instruction_cache	def in    src/c/h/flush-instruction-cache-system-dependent.h 
    Val	             result;
    REC_ALLOC1(task, result, PTR_CAST( Val, code + entrypoint));
    return           result;
}


//
static Val   do_make_package_literals_via_bytecode_interpreter   (Task* task,  Val arg)   {
    //       =================================================
    //
    // Mythryl type:   vector_of_one_byte_unts::Vector -> Vector(Chunk)
    //
    // This fn gets bound as
    //
    //      make_package_literals_via_bytecode_interpreter
    // in
    //     src/lib/compiler/execution/code-segments/code-segment.pkg
    //
    // and ultimately invoked in
    //
    //     src/lib/compiler/execution/main/execute.pkg
    //

									    ENTER_MYTHRYL_CALLABLE_C_FN("do_make_package_literals_via_bytecode_interpreter");

    return   make_package_literals_via_bytecode_interpreter (						// make_package_literals_via_bytecode_interpreter	def in    src/c/heapcleaner/make-package-literals-via-bytecode-interpreter.c
                 task,
                 GET_VECTOR_DATACHUNK_AS( Unt8*, arg ),
                 GET_VECTOR_LENGTH( arg )
             );
}


// Create a singleton record.
/*
###               "He travels the fastest who travels alone."
###
###                                 -- Rudyard Kipling
*/
static Val   do_make_single_slot_tuple   (Task* task,   Val arg)   {
    //       =========================
    //
    // Mythryl type:  Chunk -> Chunk
    //
    // This fn gets bound to   make_single_slot_tuple   in:
    //
    //     src/lib/std/src/unsafe/unsafe-chunk.pkg

									    ENTER_MYTHRYL_CALLABLE_C_FN("do_make_single_slot_tuple");

    Val               result;
    REC_ALLOC1( task, result, arg );						// REC_ALLOC1		def in    src/c/h/make-strings-and-vectors-etc.h
    return            result;
}


//
static Val   do_get_program_name_from_commandline   (Task* task,  Val arg)   {
    //       ====================================
    //
    // Mythryl type:   Void -> String
    //
    // This fn gets bound to 'get_program_name' in:
    //
    //     src/lib/std/commandline.pkg
    //

									    ENTER_MYTHRYL_CALLABLE_C_FN("do_get_program_name_from_commandline");

    return   make_ascii_string_from_c_string( task, mythryl_program_name__global );
}


//
static Val   do_get_raw_commandline_args   (Task* task,  Val arg)   {
    //       ===========================
    //
    // Mythryl type:  Void -> List(String)
    //
    // This fn gets bound to 'get_all_arguments' in:
    //
    //     src/lib/std/commandline.pkg
    //

									    ENTER_MYTHRYL_CALLABLE_C_FN("do_get_raw_commandline_args");

    return   make_ascii_strings_from_vector_of_c_strings( task, raw_args );
}


//
static Val   do_pickle_datastructure   (Task* task,  Val arg)   {
    //       =======================
    //
    // Mythryl type:  X -> vector_of_one_byte_unts::Vector
    //
    // Translate a heap chunk into a linear representation (vector of bytes).
    //
    // This fn gets bound to 'pickle_datastructure' in:
    //
    //     src/lib/std/src/unsafe/unsafe.pkg

									    ENTER_MYTHRYL_CALLABLE_C_FN("do_pickle_datastructure");

    Val  pickle =  pickle_datastructure( task, arg );								// pickle_datastructure	def in   src/c/heapcleaner/datastructure-pickler.c

    if (pickle == HEAP_VOID)   return RAISE_ERROR(task, "Attempt to pickle datastructure failed");		// XXX BUGGO FIXME Need a clearer diagnostic here.
    else                       return pickle;
}

//
static Val   do_unpickle_datastructure   (Task* task,  Val arg)   {
    //       =========================
    //
    // Mythryl type:   String -> X
    //
    // Build a Mythryl value from a string.
    //
    // This fn gets bound to 'unpickle_datastructure' in:
    //
    //     src/lib/std/src/unsafe/unsafe.pkg

									    ENTER_MYTHRYL_CALLABLE_C_FN("do_unpickle_datastructure");

    Bool	seen_error = FALSE;

    Val datastructure
	=
	unpickle_datastructure(				// unpickle_datastructure	def in    src/c/heapcleaner/datastructure-unpickler.c
	    //
	    task,
	    PTR_CAST(Unt8*, arg),
	    CHUNK_LENGTH(arg),
	    &seen_error
	);

    if (seen_error)  	return RAISE_ERROR( task, "unpickle_datastructure");
    else         	return datastructure;
}


//
static Val   do_set_sigalrm_frequency   (Task* task,  Val arg)   {
    //       ========================
    //
    // Mythryl type:   Null_Or( (Int, Int) ) -> Void
    //
    // Set the interval timer; NULL means disable the timer
    //
    // This fn gets bound as   set_sigalrm_frequency'   in:
    //
    //     src/lib/std/src/nj/set-sigalrm-frequency.pkg

									    ENTER_MYTHRYL_CALLABLE_C_FN("do_set_sigalrm_frequency");

#ifdef HAS_SETITIMER
    struct itimerval	new_itv;
    int			status;
    Val		tmp;

    if (arg == OPTION_NULL) {
													log_if("setitimer.c: Turning OFF SIGALRM interval timer\n");
        // Turn off the timer:
        //
	new_itv.it_interval.tv_sec	=
	new_itv.it_value.tv_sec		=
	new_itv.it_interval.tv_usec	=
	new_itv.it_value.tv_usec	= 0;

    } else {

        // Turn on the timer:
        //
	tmp = OPTION_GET(arg);

	new_itv.it_interval.tv_sec	=
	new_itv.it_value.tv_sec		= TUPLE_GET_INT1(tmp, 0);

	new_itv.it_interval.tv_usec	=
	new_itv.it_value.tv_usec	= GET_TUPLE_SLOT_AS_INT(tmp, 1);
													log_if("setitimer.c: Turning ON SIGALRM interval itimer, sec,usec = (%d,%d)\n",new_itv.it_value.tv_sec, new_itv.it_value.tv_usec);
    }

    RELEASE_MYTHRYL_HEAP( task->pthread, "do_set_sigalrm_frequency", arg );
	//
        status = setitimer (ITIMER_REAL, &new_itv, NULL);						// See setitimer(2), Linux Reference Manual.
	//
    RECOVER_MYTHRYL_HEAP( task->pthread, "do_set_sigalrm_frequency" );

    CHECK_RETURN_UNIT(task, status);

#elif defined(OPSYS_WIN32)

    if (arg == OPTION_NULL) {

	if (win32StopTimer())	  return HEAP_VOID;
	else                      return RAISE_ERROR( task, "win32 setitimer: couldn't stop timer");

    } else {

	Val	tmp = OPTION_GET(arg);
	int		mSecs = TUPLE_GET_INT1(tmp,0) * 1000 + GET_TUPLE_SLOT_AS_INT(tmp,1) / 1000;

	if (mSecs <= 0)   return RAISE_ERROR( task, "win32 setitimer: invalid resolution");
	else {
	    if (win32StartTimer(mSecs))	   return HEAP_VOID;
	    else                           return RAISE_ERROR( task, "win32 setitimer: couldn't start timer");
	}
    }
#else
    return RAISE_ERROR( task, "setitimer not supported");
#endif

}

#define STREQ(s1, s2)	(strcmp((s1), HEAP_STRING_AS_C_STRING(s2)) == 0)
//
static void   set_max_retained_idle_fromspace_agegroup      (Task* task,  Val  cell);
static void   clean_i_agegroups     (Task* task,  Val  cell, Val* next);
static void   clean_all_agegroups   (Task* task,             Val* next);


// One of the library bindings exported via
//     src/c/lib/heap/cfun-list.h
// and thence
//     src/c/lib/heap/libmythryl-heap.c


//
static Val   do_heapcleaner_control   (Task* task,  Val arg)   {
    //       ======================
    //
    // Mythryl type:   List( (String, Ref(Int))) -> Void
    //
    // Current control operations:
    //
    //   ("set_max_retained_idle_fromspace_agegroup", ref n)	- Set max retained-idle-fromspace agegroup to n; return old agegroup.
    //   ("DoGC", ref n)	- Clean the first "n" agegroups.
    //   ("AllGC", _)		- Clean all agegroups.
    //   ("Messages", ref 0)	- Turn cleaner messages on.
    //   ("Messages", ref n)	- Turn cleaner messages off. (n > 0)
    //
    // This fn gets bound to   cleaner_control   in:
    //
    //     src/lib/std/src/nj/heapcleaner-control.pkg

									    ENTER_MYTHRYL_CALLABLE_C_FN("do_heapcleaner_control");

    while (arg != LIST_NIL) {
      //
	Val	cmd = LIST_HEAD(arg);
	Val	op = GET_TUPLE_SLOT_AS_VAL(cmd, 0);
	Val	cell = GET_TUPLE_SLOT_AS_VAL(cmd, 1);

	arg = LIST_TAIL(arg);

	if      (STREQ("DoGC",  op))	    clean_i_agegroups   (task, cell, &arg);						// clean_i_agegroups is defined below.
	else if (STREQ("AllGC", op))	    clean_all_agegroups (task, &arg);
        //
	else if (STREQ("Messages",  op))   cleaner_messages_are_enabled__global = (TAGGED_INT_TO_C_INT(DEREF(cell)) > 0);
	else if (STREQ("LimitHeap", op))   unlimited_heap_is_enabled__global       = (TAGGED_INT_TO_C_INT(DEREF(cell)) <= 0);
        //
        else if (STREQ("set_max_retained_idle_fromspace_agegroup", op))	    set_max_retained_idle_fromspace_agegroup (task, cell);
    }

    return HEAP_VOID;
}

//
static void   set_max_retained_idle_fromspace_agegroup   (Task* task, Val arg) {
    //        ========================================
    //
									    ENTER_MYTHRYL_CALLABLE_C_FN("set_max_retained_idle_fromspace_agegroup");

    int age =  TAGGED_INT_TO_C_INT(DEREF( arg ));

    Heap*  heap  =  task->heap;

    if      (age < 0)			age = 0;
    else if (age > MAX_AGEGROUPS)	age = MAX_AGEGROUPS;

    if (age < heap->oldest_agegroup_keeping_idle_fromspace_buffers) {
	//
        // Free any retained memory regions:
        //
	for (int i = age;  i < heap->oldest_agegroup_keeping_idle_fromspace_buffers;  i++) {
	    //
	    return_multipage_ram_region_to_os( heap->agegroup[i]->saved_fromspace_ram_region );
	}
    }

    ASSIGN( arg, TAGGED_INT_FROM_C_INT(heap->oldest_agegroup_keeping_idle_fromspace_buffers) );

    heap->oldest_agegroup_keeping_idle_fromspace_buffers
	=
        age;
}

//
static void   clean_i_agegroups   (
    //        =================
    //
    Task*   task,
    Val      arg,
    Val*     next
) {
    // Force a cleaning of the given agegroups.

    Heap* heap  =  task->heap;

    int   age =  TAGGED_INT_TO_C_INT( DEREF( arg ) );

    // Clamp 'age' to sane range:
    //
    if      (age < 0)			        age =  0;
    else if (age > heap->active_agegroups)	age =  heap->active_agegroups;

    call_heapcleaner_with_extra_roots( task, age, next, NULL );				// call_heapcleaner_with_extra_roots		def in   src/c/heapcleaner/call-heapcleaner.c
}


//
static void   clean_all_agegroups   (
    //        ===================
    Task*   task,
    Val*     next
) {
    //
  call_heapcleaner_with_extra_roots(   task,							// call_heapcleaner_with_extra_roots		def in   src/c/heapcleaner/call-heapcleaner.c
                                   task->heap->active_agegroups,
                                   next,
                                   NULL
                               );
}


Val   do_breakpoint_0   (Task* task, Val arg)   { return HEAP_VOID; }
Val   do_breakpoint_1   (Task* task, Val arg)   { return HEAP_VOID; }
Val   do_breakpoint_2   (Task* task, Val arg)   { return HEAP_VOID; }
Val   do_breakpoint_3   (Task* task, Val arg)   { return HEAP_VOID; }
Val   do_breakpoint_4   (Task* task, Val arg)   { return HEAP_VOID; }
Val   do_breakpoint_5   (Task* task, Val arg)   { return HEAP_VOID; }
Val   do_breakpoint_6   (Task* task, Val arg)   { return HEAP_VOID; }
Val   do_breakpoint_7   (Task* task, Val arg)   { return HEAP_VOID; }
Val   do_breakpoint_8   (Task* task, Val arg)   { return HEAP_VOID; }
Val   do_breakpoint_9   (Task* task, Val arg)   { return HEAP_VOID; }
    //
    // See comments in   src/lib/std/src/nj/heap-debug.api



#define CFUNC(NAME, NAME2, FUNC, LIB7TYPE)	CFUNC_BIND(NAME, NAME2, FUNC, LIB7TYPE)
//
static Mythryl_Name_With_C_Function CFunTable[] = {
    //
    {"allocate_codechunk",				"allocate_codechunk",					do_allocate_codechunk,						"Int -> rw_vector_of_one_byte_unts::Rw_Vector"},
    {"breakpoint_0",					"breakpoint_0",						do_breakpoint_0,						"Void -> Void"},
    {"breakpoint_1",					"breakpoint_1",						do_breakpoint_1,						"Void -> Void"},
    {"breakpoint_2",					"breakpoint_2",						do_breakpoint_2,						"Void -> Void"},
    {"breakpoint_3",					"breakpoint_3",						do_breakpoint_3,						"Void -> Void"},
    {"breakpoint_4",					"breakpoint_4",						do_breakpoint_4,						"Void -> Void"},
    {"breakpoint_5",					"breakpoint_5",						do_breakpoint_5,						"Void -> Void"},
    {"breakpoint_6",					"breakpoint_6",						do_breakpoint_6,						"Void -> Void"},
    {"breakpoint_7",					"breakpoint_7",						do_breakpoint_7,						"Void -> Void"},
    {"breakpoint_8",					"breakpoint_8",						do_breakpoint_8,						"Void -> Void"},
    {"breakpoint_9",					"breakpoint_9",						do_breakpoint_9,						"Void -> Void"},
    {"check_agegroup0_overrun_tripwire_buffer",		"check_agegroup0_overrun_tripwire_buffer",		do_check_agegroup0_overrun_tripwire_buffer,			"String -> Void"},
    {"cleaner_control",					"heapcleaner_control",					do_heapcleaner_control,						"List( (String, Ref(Int)) ) -> Void"},
    {"commandline_args",				"commandline_args",					do_get_commandline_args,					"Void -> List String"},
    {"concatenate_two_tuples",				"concatenate_two_tuples",				do_concatenate_two_tuples,					"(Chunk, Chunk) -> Chunk"},
    {"debug",						"debug",						do_debug,							"String -> Void"},
    {"dummy",						"dummy",						do_dummy,							"String -> Void"},
    {"disable_debug_logging",				"disable_debug_logging",				do_disable_debug_logging,					"Void -> Void"},
    {"enable_debug_logging",				"enable_debug_logging",					do_enable_debug_logging,					"Void -> Void"},
    {"export_heap",					"export_heap",						do_export_heap,							"String -> Bool"},
    {"get_platform_property",				"get_platform_property",				do_get_platform_property,					"String -> Null_Or String"},
    {"interval_tick__unimplemented",			"interval_tick__unimplemented",				do_interval_tick__unimplemented,				"Void -> (Int, Int)"},	// Currently UNIMPLEMENTED
    {"dump_gen0",					"dump_gen0",						do_dump_gen0,							"String -> Void"},
    {"dump_gens",					"dump_gens",						do_dump_gens,							"String -> Void"},
    {"dump_hugechunk_stuff",				"dump_hugechunk_stuff",					do_dump_hugechunk_stuff,					"String -> Void"},
    {"dump_task",					"dump_task",						do_dump_task,							"String -> Void"},
    {"dump_whatever",					"dump_whatever",					do_dump_whatever,						"String -> Void"},
    {"make_codechunk_executable",			"make_codechunk_executable",				do_make_codechunk_executable,					"(Vector_Of_One_Byte_Unts, Int) -> Chunk -> Chunk"},
    {"make_package_literals_via_bytecode_interpreter",	"make_package_literals_via_bytecode_interpreter",	do_make_package_literals_via_bytecode_interpreter,		"vector_of_one_byte_unts::Vector -> Ovec"},
    {"make_single_slot_tuple",				"make_single_slot_tuple",				do_make_single_slot_tuple,					"Chunk -> Chunk"},
    {"pickle_datastructure",				"pickle_datastructure",					do_pickle_datastructure,					"X -> vector_of_one_byte_unts::Vector"},
    {"program_name_from_commandline",			"program_name_from_commandline",			do_get_program_name_from_commandline,				"Void -> String"},
    {"raw_commandline_args",				"raw_commandline_args",					do_get_raw_commandline_args,					"Void -> List String"},
    {"set_sigalrm_frequency",				"set_sigalrm_frequency",				do_set_sigalrm_frequency,					"Null_Or (Int, Int) -> Null_Or (Int, Int)"},
    {"spawn_to_disk",					"spawn_to_disk",					do_spawn_to_disk,						"(String, (List(String) -> Void)) -> Void"},
    {"unpickle_datastructure",				"unpickle_datastructure",				do_unpickle_datastructure,					"vector_of_one_byte_unts::Vector -> X"},
    //
    CFUNC_NULL_BIND
};
#undef CFUNC


// The Runtime library.
//
// Our record                Libmythryl_Heap
// gets compiled into        src/c/lib/mythryl-callable-c-libraries-list.h
// and thus ultimately       mythryl_callable_c_libraries__local []
// in                        src/c/lib/mythryl-callable-c-libraries.c
//
Mythryl_Callable_C_Library	    Libmythryl_Heap = {						// Mythryl_Callable_C_Library		def in    src/c/h/mythryl-callable-c-libraries.h
    //                              ===============
    "heap",
    "1.0",
    "December 15, 1994",
    NULL,
    CFunTable
};



// COPYRIGHT (c) 1994 AT&T Bell Laboratories.
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
