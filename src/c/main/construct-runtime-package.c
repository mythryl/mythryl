// construct-runtime-package.c
//
// During bootstrap the   runtime   package provides the
// initial bridge between Mythryl-level and C-level code.
// This package is defined on the Mythryl side, in
//
//     src/lib/core/init/runtime.api
//     src/lib/core/init/runtime.pkg			// A dummy place-holder containing no actual code.
//
// but implemented here on the C side with assists
// from the platform-specific files
//
//     src/c/machine-dependent/prim.intel32.asm
//     src/c/machine-dependent/prim.sparc32.asm
//     src/c/machine-dependent/prim.pwrpc32.asm
//     src/c/machine-dependent/prim.intel32.masm
//     
// A special hack in
//
//     src/c/main/load-compiledfiles.c
//
// substitutes (during linking) the
//
//     runtime_package__global
//
// which we create here for the (dummy) file
//
//     src/lib/core/init/runtime.pkg.compiled
//
// In particular, this mechanism provides
// Mythryl-level access via
//
//     runtime::asm::find_cfun
//
// and the resulting callchain
//
//     find_cfun_asm				in (for example)	src/c/machine-dependent/prim.intel32.asm
// ->  REQUEST_FIND_CFUN			in			src/c/main/run-mythryl-code-and-runtime-eventloop.c
// ->  find_mythryl_callable_c_function		in    			src/c/lib/mythryl-callable-c-libraries.c
// 
// to the two dozen or so Mythryl-callable C libraries listed in
//
//     src/c/lib/mythryl-callable-c-libraries-list.h
//
//     


#include "../mythryl-config.h"

#include "runtime-base.h"
#include "architecture-and-os-names-system-dependent.h"
#include "runtime-values.h"
#include "heap-tags.h"
#include "runtime-globals.h"
#include "make-strings-and-vectors-etc.h"
#include "runtime-configuration.h"
#include "mythryl-callable-cfun-hashtable.h"

#ifdef SIZES_C_64_MYTHRYL_32
    void patch_static_heapchunk_32_bit_addresses ();
#endif

#ifndef SIZES_C_64_MYTHRYL_32

typedef struct {
	Val	desc;
	char		*s;
	Val	len;
} lib7_string_t;

										// STRING_TAGWORD			def in   src/c/h/heap-tags.h
										// TAGGED_INT_FROM_C_INT			def in   src/c/h/runtime-values.h 

#define LIB7_STRING(id, s)					\
    lib7_string_t id = {					\
	STRING_TAGWORD,						\
	s,							\
	TAGGED_INT_FROM_C_INT(sizeof(s)-1)				\
    }

										// CONCAT				def in   src/c/h/runtime-base.h
										// REFCELL_TAGWORD			def in   src/c/h/heap-tags.h
										// PTR_CAST				def in   src/c/h/runtime-values.h 
// Exceptions are identified by Ref(String) values:
//
#define LIB7_EXNID(ex,name)					\
    LIB7_STRING(CONCAT(ex,_s), name);				\
    Val CONCAT(ex,_id0) [2] = {					\
	REFCELL_TAGWORD,					\
	PTR_CAST( Val, &(CONCAT(ex,_s).s))			\
    }

										// MAKE_TAGWORD				def in   src/c/h/heap-tags.h
										// PAIRS_AND_RECORDS_TAGWORD		def in   src/c/h/heap-tags.h

#define ASM_CLOSURE(name)					\
    extern Val CONCAT(name,_asm)[];				\
    Val CONCAT(name,_v)[2] = {					\
	MAKE_TAGWORD(1,PAIRS_AND_RECORDS_BTAG),			\
	PTR_CAST( Val, CONCAT(name,_asm))			\
    }

#else // SIZES_C_64_MYTHRYL_32

// When the size of Vunt is bigger
// than the size of an Vunt, we need
// to dynamically patch the static Mythryl chunks.

typedef struct {
	Val	desc;
	Val	s;
	Val	len;
} lib7_string_t;

#define LIB7_STRING(id,s)					\
    static char CONCAT(id,_data)[] = s;				\
    lib7_string_t id = {					\
	STRING_TAGWORD, HEAP_VOID, TAGGED_INT_FROM_C_INT(sizeof(s)-1)	\
    }

#define PATCH_LIB7_STRING(id)					\
    id.s = PTR_CAST( Val, CONCAT(id,_data))

// Exceptions are identified by (string ref) values:
//
#define LIB7_EXNID(ex,name)					\
    LIB7_STRING(CONCAT(ex,_s),name);				\
    Val CONCAT(ex,_id0) [2] = { REFCELL_TAGWORD, }

#define PATCH_LIB7_EXNID(ex)					\
    PATCH_LIB7_STRING(CONCAT(ex,_s));				\
    CONCAT(ex,_id0)[1] = PTR_CAST( Val, &(CONCAT(ex,_s).s))

#define ASM_CLOSURE(name)					\
    extern Val CONCAT(name,_asm)[];				\
    Val CONCAT(name,_v)[2] = {					\
	MAKE_TAGWORD(1, PAIRS_AND_RECORDS_BTAG),		\
    }

#define PATCH_ASM_CLOSURE(name)					\
    CONCAT(name,_v)[1] = PTR_CAST( Val, CONCAT(name,_asm))

#endif


#if (CALLEE_SAVED_REGISTERS_COUNT > 0)
    #define ASM_CONT(name) 					\
	extern Val CONCAT(name,_asm)[];				\
	Val* CONCAT(name,_c) = (Val*) (CONCAT(name,_asm))
#else
    #define ASM_CONT(name)					\
	ASM_CLOSURE(name);					\
	Val* CONCAT(name,_c) = (Val*)(CONCAT(name,_v)+1)
#endif

// Machine identification strings:
//
LIB7_STRING( machine_id, ARCHITECTURE_NAME );

// The following are assembly-language functions
// defined in the platform-specific files
//
//     src/c/machine-dependent/prim.intel32.asm
//     src/c/machine-dependent/prim.sparc32.asm
//     src/c/machine-dependent/prim.pwrpc32.asm
//     src/c/machine-dependent/prim.intel32.masm
//
ASM_CLOSURE(make_typeagnostic_rw_vector);
ASM_CLOSURE(find_cfun);
ASM_CLOSURE(call_cfun);
ASM_CLOSURE(make_unt8_rw_vector);
ASM_CLOSURE(make_float64_rw_vector);
ASM_CLOSURE(make_string);
ASM_CLOSURE(make_vector);
ASM_CLOSURE(floor);
ASM_CLOSURE(logb);
ASM_CLOSURE(scalb);
ASM_CLOSURE(try_lock);
ASM_CLOSURE(unlock);
ASM_CLOSURE(handle_uncaught_exception_closure);								// See  handle_uncaught_exception_closure_v                          in   src/c/h/runtime-globals.h
													// and  handle_uncaught_exception_closure_asm                        in   src/c/machine-dependent/prim*asm 

ASM_CONT(return_to_c_level);										// Invokes   return_to_c_level_asm                                   in   src/c/machine-dependent/prim*asm
ASM_CONT(return_from_signal_handler);									// Invokes   return_from_signal_handler                              in   src/c/machine-dependent/prim*asm
ASM_CONT(return_from_software_generated_periodic_event_handler);					// Invokes   return_from_software_generated_periodic_event_handler   in   src/c/machine-dependent/prim*asm



#define DEFINE_VOID_REFCELL(z)	Val z[2] = {REFCELL_TAGWORD, HEAP_VOID}

DEFINE_VOID_REFCELL( this_fn_profiling_hook_refcell__global			);
DEFINE_VOID_REFCELL( pervasive_package_pickle_list_refcell__global		);
DEFINE_VOID_REFCELL( posix_interprocess_signal_handler_refcell__global		);
DEFINE_VOID_REFCELL( software_generated_periodic_events_handler_refcell__global	);
DEFINE_VOID_REFCELL( software_generated_periodic_events_switch_refcell__global	);
DEFINE_VOID_REFCELL( software_generated_periodic_event_interval_refcell__global	);
DEFINE_VOID_REFCELL( microthread_switch_lock__global				);

#undef DEFINE_VOID_REFCELL



// This global will point to a carefully constructed fake
// which looks like a normal compiled package from the
// Mythryl side but actually links to compiled C code -- see
// the hack in
//	
//     src/c/main/load-compiledfiles.c
//
Val		runtime_package__global = HEAP_VOID;
#ifdef ASM_MATH
    Val		mathvec__global = HEAP_VOID;
#endif

// Aggregate vectors of length zero:
//
const char zero_length_string_global_data[ 1 ] =  { 0 };
Val        zero_length_string__global[ 3 ]      =  { STRING_TAGWORD,  PTR_CAST( Val,  zero_length_string_global_data ), TAGGED_INT_FROM_C_INT(0) };
Val        zero_length_vector__global[ 3 ]      =  { TYPEAGNOSTIC_RO_VECTOR_TAGWORD, HEAP_VOID, TAGGED_INT_FROM_C_INT(0) };

LIB7_EXNID( divide_exception__global,	"DIVIDE_BY_ZERO"	);
LIB7_EXNID( overflow_exception__global,	"OVERFLOW"		);
LIB7_EXNID( runtime_exception__global, 	"RUNTIME_EXCEPTION"	);

extern Val externlist0 [];

#ifdef ASM_MATH
LIB7_EXNID(ln__global,"LN__GLOBAL");
LIB7_EXNID(sqrt__global,"SQRT__GLOBAL");
#endif


// A table of pointers to global C variables that
// are potential garbage-collection roots:
//
Val* c_roots__global[ MAX_C_HEAPCLEANER_ROOTS ] = {
    //
    &runtime_package__global,
    pervasive_package_pickle_list_refcell__global +1,
    posix_interprocess_signal_handler_refcell__global +1,
    software_generated_periodic_events_handler_refcell__global +1,
#ifdef ASM_MATH
    &mathvec__global,
#else
    NULL,
#endif
    NULL, NULL
};

#ifdef ASM_MATH
    //
    int c_roots_count__global = 5;
#else
    int	c_roots_count__global = 4;
#endif



void   construct_runtime_package__global   (Task* task) {
    //
    // This fn gets called exactly one place, in
    //
    //     src/c/main/load-compiledfiles.c
    //
    Val	runtime_asm;
    Val runtime;

    #ifdef SIZES_C_64_MYTHRYL_32
	patch_static_heapchunk_32_bit_addresses ();
    #endif

    // Here we populate runtime::asm::* with pointers to the
    // platform-specific assembly language routines in the
    // appropriate one of:
    //
    //     src/c/machine-dependent/prim.intel32.asm
    //     src/c/machine-dependent/prim.sparc32.asm
    //     src/c/machine-dependent/prim.pwrpc32.asm
    //     src/c/machine-dependent/prim.intel32.masm
    //
    // These provide the actual implementations for the
    //
    //     runtime::asm::*
    //
    // functions specified in
    // 
    //      src/lib/core/init/runtime.api
    //      src/lib/core/init/runtime.pkg
    //
    // 'runtime_asm' gets slotted into the 'runtime' record created below,
    // which in turn gets published in
    //
    //     runtime_package__global
    //
    // for eventual use in
    //
    //     src/c/main/load-compiledfiles.c
    //
    #define RUNTIME_ASM_SIZE	12
	set_slot_in_nascent_heapchunk(task,  0, MAKE_TAGWORD(RUNTIME_ASM_SIZE, PAIRS_AND_RECORDS_BTAG));
	set_slot_in_nascent_heapchunk(task,  1, PTR_CAST( Val,  make_typeagnostic_rw_vector_v     +1));
	set_slot_in_nascent_heapchunk(task,  2, PTR_CAST( Val,  find_cfun_v +1));
	set_slot_in_nascent_heapchunk(task,  3, PTR_CAST( Val,  call_cfun_v +1));
	set_slot_in_nascent_heapchunk(task,  4, PTR_CAST( Val,  make_unt8_rw_vector_v  +1));
	set_slot_in_nascent_heapchunk(task,  5, PTR_CAST( Val,  make_float64_rw_vector_v  +1));
	set_slot_in_nascent_heapchunk(task,  6, PTR_CAST( Val,  make_string_v  +1));
	set_slot_in_nascent_heapchunk(task,  7, PTR_CAST( Val,  make_vector_v  +1));
	set_slot_in_nascent_heapchunk(task,  8, PTR_CAST( Val,  floor_v     +1));
	set_slot_in_nascent_heapchunk(task,  9, PTR_CAST( Val,  logb_v      +1));
	set_slot_in_nascent_heapchunk(task, 10, PTR_CAST( Val,  scalb_v     +1));
	set_slot_in_nascent_heapchunk(task, 11, PTR_CAST( Val,  try_lock_v  +1));
	set_slot_in_nascent_heapchunk(task, 12, PTR_CAST( Val,  unlock_v    +1));
	runtime_asm = commit_nascent_heapchunk(task, RUNTIME_ASM_SIZE);
    #undef RUNTIME_ASM_SIZE

    // Allocate the CStruct.  This is a set of heap values
    // which will be visible in both the C and Mythryl worlds.
    // See
    //     src/lib/core/init/runtime.api
    //     src/lib/core/init/runtime.pkg
    // 
    // for the Mythryl window onto this, and the hack in
    //
    //     src/c/main/load-compiledfiles.c
    //
    // for part of the magic that makes it happen.
    //
    #define RUNTIME_SIZE	12
	set_slot_in_nascent_heapchunk(task,  0, MAKE_TAGWORD(RUNTIME_SIZE, PAIRS_AND_RECORDS_BTAG));
	set_slot_in_nascent_heapchunk(task,  1, runtime_asm);
	set_slot_in_nascent_heapchunk(task,  2, DIVIDE_EXCEPTION__GLOBAL);
	set_slot_in_nascent_heapchunk(task,  3, OVERFLOW_EXCEPTION__GLOBAL);
	set_slot_in_nascent_heapchunk(task,  4, RUNTIME_EXCEPTION__GLOBAL);
	set_slot_in_nascent_heapchunk(task,  5, THIS_FN_PROFILING_HOOK_REFCELL__GLOBAL);				// this_fn_profiling_hook_refcell__global			in  src/lib/core/init/runtime.api
	set_slot_in_nascent_heapchunk(task,  6, SOFTWARE_GENERATED_PERIODIC_EVENTS_SWITCH_REFCELL__GLOBAL);		// software_generated_periodic_events_switch_refcell__global	in  src/lib/core/init/runtime.api
	set_slot_in_nascent_heapchunk(task,  7, SOFTWARE_GENERATED_PERIODIC_EVENT_INTERVAL_REFCELL__GLOBAL);		// software_generated_periodic_event_interval_refcell__global	in  src/lib/core/init/runtime.api
	set_slot_in_nascent_heapchunk(task,  8, SOFTWARE_GENERATED_PERIODIC_EVENTS_HANDLER_REFCELL__GLOBAL);		// software_generated_periodic_event_handler_refcell__global	in  src/lib/core/init/runtime.api
	set_slot_in_nascent_heapchunk(task,  9, MICROTHREAD_SWITCH_LOCK__GLOBAL);					// microthread_switch_lock__global				in  src/lib/core/init/runtime.api
	set_slot_in_nascent_heapchunk(task, 10, PERVASIVE_PACKAGE_PICKLE_LIST_REFCELL__GLOBAL);				// pervasive_package_pickle_list__global			in  src/lib/core/init/runtime.api
	set_slot_in_nascent_heapchunk(task, 11, POSIX_INTERPROCESS_SIGNAL_HANDLER_REFCELL__GLOBAL );			// posix_interprocess_signal_handler_refcell__global		in  src/lib/core/init/runtime.api
	set_slot_in_nascent_heapchunk(task, 12, ZERO_LENGTH_VECTOR__GLOBAL);						// zero_length_vector__global					in  src/lib/core/init/runtime.api
	runtime = commit_nascent_heapchunk(task, RUNTIME_SIZE);
    #undef RUNTIME_SIZE

    runtime_package__global =  make_one_slot_record( task, runtime );
	//
	// Make 1-slot SRECORD containing the runtime
	// and publish it in
	//
	//     runtime_package__global
	//
	// for eventual use by   src/c/main/load-compiledfiles.c

    #ifdef ASM_MATH
        #define MATHVEC_SZ	8
	set_slot_in_nascent_heapchunk(task,  0, MAKE_TAGWORD(MATHVEC_SZ, PAIRS_AND_RECORDS_BTAG));
	set_slot_in_nascent_heapchunk(task,  1, LN_ID__GLOBAL);
	set_slot_in_nascent_heapchunk(task,  2, SQRT_ID__GLOBAL);
	set_slot_in_nascent_heapchunk(task,  3, PTR_CAST( Val,  arctan_v +1));
	set_slot_in_nascent_heapchunk(task,  4, PTR_CAST( Val,  cos_v    +1));
	set_slot_in_nascent_heapchunk(task,  5, PTR_CAST( Val,  exp_v    +1));
	set_slot_in_nascent_heapchunk(task,  6, PTR_CAST( Val,  ln_v     +1));
	set_slot_in_nascent_heapchunk(task,  7, PTR_CAST( Val,  sin_v    +1));
	set_slot_in_nascent_heapchunk(task,  8, PTR_CAST( Val,  sqrt_v   +1));
	mathvec__global = commit_nascent_heapchunk(task, MATHVEC_SZ);
    #endif

}								// fun construct_runtime_package__global



void   publish_runtime_package_contents   ()   {
    //
    // All Mythryl 'executables' such as
    //
    //     bin/mythryld
    //     bin/mythryl-lex
    //     bin/mythryl-yacc
    //
    // are actually heap images with an initial "shebang" line
    //
    //     #!/usr/bin/mythryl-runtime-intel32
    //
    // which makes them 'scripts' to the host operating system.
    //
    // Consequently, saving such heap images to disk and later
    // restoring them to executable form in memory is critically
    // important to us.
    //
    // Because the Mythryl heap may contain pointers to any of the
    // C functions in the libraries listed in
    //
    //     src/c/lib/mythryl-callable-c-libraries-list.h
    //
    // and because the exact addresses of these functions may change
    // each time mythryl-runtime-intel32 is recompiled, when saving out a heap image
    // we must replace all heap references to such functions by
    // abstract names for them, and then do the reverse when loading
    // the heap image back into memory.
    //
    // To support doing so, we maintain in
    //
    //     src/c/heapcleaner/mythryl-callable-cfun-hashtable.c
    //
    // a global hashtable of C-level resources accessible from
    // Mythryl-level code.
    //
    // Most of the entries in this hashtable are created by calls to
    //
    //     publish_cfun ()
    // in
    //     set_up_list_of_c_functions_callable_from_mythryl   ()
    // from
    //     src/c/lib/mythryl-callable-c-libraries.c
    //
    // entering all the vanilla C libraries, but since the runtime
    // package functions, exceptions and refcells are also referenced
    // by the Mythryl heap, they must also be entered into the hashtable.
    //
    // Doing so is our job here.
    //
    // We are called only once, very early in runtime's main() fn in:
    //
    //     src/c/main/runtime-main.c
    //


    // Miscellaneous stuff:
    //
    publish_cfun("handle_uncaught_exception_closure",	PTR_CAST( Val, handle_uncaught_exception_closure_v+1));			// See handle_uncaught_exception_closure_v    in  src/c/h/runtime-globals.h
																// and handle_uncaught_exception_closure_asm  in  src/c/machine-dependent/prim*asm 

    publish_cfun ("return_to_c_level",	PTR_CAST( Val, return_to_c_level_c));							// See return_to_c_level_asm                  in  src/c/machine-dependent/prim*asm 
    #if (CALLEE_SAVED_REGISTERS_COUNT == 0)
	publish_cfun ("return_to_c_level_asm", PTR_CAST( Val, return_to_c_level_asm));
    #endif

    // Here we populate runtime::asm::* in
    //
    //     src/lib/core/init/runtime.pkg
    //
    // with assembly code functions from one
    // of the platform-specific files:
    //
    //     src/c/machine-dependent/prim.sparc32.asm
    //     src/c/machine-dependent/prim.pwrpc32.asm
    //     src/c/machine-dependent/prim.intel32.asm	
    //     src/c/machine-dependent/prim.intel32.masm
    //
    // These definitions are never directly referenced
    // from either the C or Mythryl levels.  They are
    // however needed by the heap export/import logic
    // which we use to construct   bin/mythryld   etc,
    // specifically
    //     add_cfun_to_heapfile_cfun_table
    // in
    //     src/c/heapcleaner/mythryl-callable-cfun-hashtable.c
    //
    // NB: Renaming in left-hand column is nontrivial, you'll
    // get 'runtime does not provide X' errors from mythryld.
    // Use publish_cfun2() and see Hashtable_Entry comments in
    // src/c/heapcleaner/mythryl-callable-cfun-hashtable.c
    //
    publish_cfun( "runtime::asm::make_polymorphic_rw_vector",	PTR_CAST( Val, make_typeagnostic_rw_vector_v+1));
    publish_cfun( "runtime::asm::find_cfun",			PTR_CAST( Val, find_cfun_v+1));
    publish_cfun( "runtime::asm::call_cfun",			PTR_CAST( Val, call_cfun_v+1));
    publish_cfun( "runtime::asm::make_unt8_rw_vector",		PTR_CAST( Val, make_unt8_rw_vector_v+1));
    publish_cfun( "runtime::asm::make_float64_rw_vector",	PTR_CAST( Val, make_float64_rw_vector_v+1));
    publish_cfun( "runtime::asm::make_string",			PTR_CAST( Val, make_string_v+1));
    publish_cfun( "runtime::asm::make_vector",			PTR_CAST( Val, make_vector_v+1));
    publish_cfun( "runtime::asm::floor",			PTR_CAST( Val, floor_v+1));
    publish_cfun( "runtime::asm::logb",				PTR_CAST( Val, logb_v+1));
    publish_cfun( "runtime::asm::scalb",			PTR_CAST( Val, scalb_v+1));
    publish_cfun( "runtime::asm::try_lock",			PTR_CAST( Val, try_lock_v+1));
    publish_cfun( "runtime::asm::unlock",			PTR_CAST( Val, unlock_v+1));

    // runtime:  This exports various refcells and exceptions
    // used to communicate between the C/assembly level and
    // and the Mythryl level.
    //
    // The exceptions are generated at the C/assembly level
    // to be handled at the Mythryl level.
    //
    // The refcells are hooks settable at the Mythryl level
    // to control/customize the C/assembly level handling of
    // various situations.
    //
    // See  src/lib/core/init/runtime.pkg
    //      src/lib/core/init/runtime.api
    //
    //
    // NB: Renaming in left-hand column is nontrivial, you'll
    // get 'runtime does not provide X' errors from mythryld.
    // Use publish_cfun2() and see Hashtable_Entry comments in
    // src/c/heapcleaner/mythryl-callable-cfun-hashtable.c
    //
    publish_cfun( "runtime::divide_exception",						DIVIDE_EXCEPTION__GLOBAL					);
    publish_cfun( "runtime::overflow_exception",					OVERFLOW_EXCEPTION__GLOBAL					);
    publish_cfun( "runtime::runtime_exception",						RUNTIME_EXCEPTION__GLOBAL					);
    publish_cfun( "runtime::machine_id",						PTR_CAST( Val, machine_id.s)					);
    publish_cfun( "runtime::pervasive_package_pickle_list_refcell",			PERVASIVE_PACKAGE_PICKLE_LIST_REFCELL__GLOBAL			);
    publish_cfun( "runtime::posix_interprocess_signal_handler_refcell",			POSIX_INTERPROCESS_SIGNAL_HANDLER_REFCELL__GLOBAL		);
    publish_cfun( "runtime::zero_length_vector",					ZERO_LENGTH_VECTOR__GLOBAL					);
    publish_cfun( "runtime::profiling_hook_refcell",					THIS_FN_PROFILING_HOOK_REFCELL__GLOBAL				);
    publish_cfun( "runtime::software_generated_periodic_events_handler_refcell",   	SOFTWARE_GENERATED_PERIODIC_EVENTS_HANDLER_REFCELL__GLOBAL	);
    publish_cfun( "runtime::software_generated_periodic_events_switch_refcell",		SOFTWARE_GENERATED_PERIODIC_EVENTS_SWITCH_REFCELL__GLOBAL	);
    publish_cfun( "runtime::software_generated_periodic_event_interval_refcell",	SOFTWARE_GENERATED_PERIODIC_EVENT_INTERVAL_REFCELL__GLOBAL	);
    publish_cfun( "runtime::active_pthreads_count_refcell",				MICROTHREAD_SWITCH_LOCK__GLOBAL					);
        //
	// I'd like to comment out the above line, but if I do I get
	//
	//     bin/mythryld: Fatal error:  Run-time system does not provide "runtime::active_pthreads_count_refcell"
	//
	// even though the string       active_hostthreads_count   appears
	// nowhere else in the sourcecode.   Apparently the compiler is
	// passing it binary-to-binary somehow. Thpt. -- 2011-11-29 CrT

    // Null string:
    //
    publish_cfun ("string0",			ZERO_LENGTH_STRING__GLOBAL);

    // I don't think ASM_MATH is ever defined, and I can't find any code which would actually use this stuff if it was. Needs to be fixed or deleted.  -- 2010-12-15 CrT  XXX BUGGO FIXME
    #if defined(ASM_MATH)
	//
	// mathvec__global
	//
	publish_cfun( "math::asm::ln_id",		LN_ID__GLOBAL			);
	publish_cfun( "math::asm::sqrt_id",		SQRT_ID__GLOBAL			);
	publish_cfun( "math::asm::arctan",		PTR_CAST( Val, arctan_v+1)	);
	publish_cfun( "math::asm::cos",			PTR_CAST( Val, cos_v   +1)	);
	publish_cfun( "math::asm::exp",			PTR_CAST( Val, exp_v   +1)	);
	publish_cfun( "math::asm::ln",			PTR_CAST( Val, ln_v    +1)	);
	publish_cfun( "math::asm::sin",			PTR_CAST( Val, sin_v   +1)	);
	publish_cfun( "math::asm::sqrt",		PTR_CAST( Val, sqrt_v  +1)	);
    #endif

}									// fun publish_runtime_package_contents.



#ifdef SIZES_C_64_MYTHRYL_32

    void   patch_static_heapchunk_32_bit_addresses   () {
	//
	// On machines where the size of  Vunt
	// is bigger than the size of a       Vunt
	// we need to dynamically patch
	// the static heap chunks:

	PATCH_LIB7_STRING(machine_id);

	PATCH_LIB7_EXNID(      div_exception__global	);
	PATCH_LIB7_EXNID( overflow_exception__global	);
	PATCH_LIB7_EXNID(  runtime_exception__global	);

	PATCH_ASM_CLOSURE( make_typeagnostic_rw_vector	);
	PATCH_ASM_CLOSURE( find_cfun	);
	PATCH_ASM_CLOSURE( call_cfun	);
	PATCH_ASM_CLOSURE( make_unt8_rw_vector	);
	PATCH_ASM_CLOSURE( make_float64_rw_vector	);
	PATCH_ASM_CLOSURE( make_string	);
	PATCH_ASM_CLOSURE( make_vector	);
	PATCH_ASM_CLOSURE( floor	);
	PATCH_ASM_CLOSURE( logb		);
	PATCH_ASM_CLOSURE( scalb	);
	PATCH_ASM_CLOSURE( try_lock	);
	PATCH_ASM_CLOSURE( unlock	);
	PATCH_ASM_CLOSURE( handle_uncaught_exception_closure	);

	#if (CALLEE_SAVED_REGISTERS_COUNT <= 0)
	    //
	    PATCH_ASM_CLOSURE( return_to_c_level	);
	    PATCH_ASM_CLOSURE( return_from_signal_handler	);
	#endif
    }							// fun patch_static_heapchunk_32_bit_addresses

#endif // SIZES_C_64_MYTHRYL_32


// COPYRIGHT (c) 1993 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2013,
// released per terms of SMLNJ-COPYRIGHT.


