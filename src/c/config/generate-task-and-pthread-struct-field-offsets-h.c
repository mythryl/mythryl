// generate-task-and-pthread-struct-field-offsets-h.c
//
// This C program analyses the Task and Pthread structs from (respectively)
//
//     src/c/h/task.h
//     src/c/h/pthread.h
//
// and makes the
// 
//     Pthread* pthread;
//     Val*	heap_allocation_limit;
//     Val*	heap_allocation_pointer;
//              ...
//
// fields in them available as a set of #defined byte offsets
//
//     #define     pthread_byte_offset_in_task_struct  4
//     #define heap_allocation_pointer_byte_offset_in_task_struct  8
//     #define  heap_allocation_limit_byte_offset_in_task_struct 12
//
// in the file
//
//     src/c/o/task-and-pthread-struct-field-offsets--autogenerated.h
//                          ...
// for use in the files
// 
//    src/c/machine-dependent/prim.sparc32.asm
//    src/c/machine-dependent/prim.intel32.asm
//    src/c/machine-dependent/prim.pwrpc32.asm

#include "../config.h"

#include "runtime-base.h"
#include "pthread.h"
#include "task.h"
#include "header-file-autogeneration-stuff.h"

#define    TASK_FIELD_BYTE_OFFSET(fld)	(((Punt)&(   task_union.s.fld)) - (Punt)&(   task_union.b[0]))
#define PTHREAD_FIELD_BYTE_OFFSET(fld)	(((Punt)&(pthread_union.s.fld)) - (Punt)&(pthread_union.b[0]))

#define PRINT_PTHREAD_FIELD_BYTE_OFFSET(fieldname, field)   fprintf(fd, "#define %s_byte_offset_in_pthread_struct %ld\n", (fieldname), (long int) PTHREAD_FIELD_BYTE_OFFSET(field))
#define    PRINT_TASK_FIELD_BYTE_OFFSET(fieldname, field)   fprintf(fd, "#define %s_byte_offset_in_task_struct %ld\n",    (fieldname), (long int)    TASK_FIELD_BYTE_OFFSET(field))


int   main   (void) {
    //====
    //
    char*       filename      =  "task-and-pthread-struct-field-offsets--autogenerated.h";
    char*       unique_string =  "TASK_AND_PTHREAD_STRUCT_FIELD_OFFSETS__AUTOGENERATED_H";
    char*       progname      =  "src/c/config/generate-task-and-pthread-struct-field-offsets-h.c";

    union { Pthread s; char b[sizeof(Pthread)]; } pthread_union;
    union { Task    s; char b[sizeof(Task)];    } task_union;

    FILE* fd =   start_generating_header_file( filename, unique_string, progname );			// start_generating_header_file		is from   src/c/config/start-and-finish-generating-header-file.c

    PRINT_TASK_FIELD_BYTE_OFFSET( "pthread",						pthread						);
    PRINT_TASK_FIELD_BYTE_OFFSET( "heap_allocation_pointer",				heap_allocation_pointer				);
    PRINT_TASK_FIELD_BYTE_OFFSET( "heap_allocation_limit",				heap_allocation_limit				);
    PRINT_TASK_FIELD_BYTE_OFFSET( "heap_changelog",					heap_changelog					);
    PRINT_TASK_FIELD_BYTE_OFFSET( "argument", 						argument					);
    PRINT_TASK_FIELD_BYTE_OFFSET( "fate", 						fate						);
    PRINT_TASK_FIELD_BYTE_OFFSET( "closure", 						closure						);
    PRINT_TASK_FIELD_BYTE_OFFSET( "link_register",					link_register					);
    PRINT_TASK_FIELD_BYTE_OFFSET( "program_counter",					program_counter					);
    PRINT_TASK_FIELD_BYTE_OFFSET( "exception_fate",					exception_fate					);
    PRINT_TASK_FIELD_BYTE_OFFSET( "thread",						thread						);
    PRINT_TASK_FIELD_BYTE_OFFSET( "callee_saved_register_0", 				callee_saved_registers[0]			);
    PRINT_TASK_FIELD_BYTE_OFFSET( "callee_saved_register_1", 				callee_saved_registers[1]			);
    PRINT_TASK_FIELD_BYTE_OFFSET( "callee_saved_register_2",				callee_saved_registers[2]			);

    #ifdef SOFTWARE_GENERATED_PERIODIC_EVENTS
	//
	PRINT_TASK_FIELD_BYTE_OFFSET( "real_heap_allocation_limit",			real_heap_allocation_limit				);	// Nowhere referenced.
	PRINT_TASK_FIELD_BYTE_OFFSET( "software_generated_periodic_event_is_pending",	software_generated_periodic_event_is_pending	);	// Nowhere referenced.
	PRINT_TASK_FIELD_BYTE_OFFSET( "in_software_generated_periodic_event_handler",	in_software_generated_periodic_event_handler	);	// Nowhere referenced.
	//
    #endif

    PRINT_PTHREAD_FIELD_BYTE_OFFSET( "executing_mythryl_code",				executing_mythryl_code				);
    PRINT_PTHREAD_FIELD_BYTE_OFFSET( "ccall_limit_pointer_mask",			ccall_limit_pointer_mask			);	// Nowhere referenced.
    PRINT_PTHREAD_FIELD_BYTE_OFFSET( "posix_signal_pending", 				posix_signal_pending				);
    PRINT_PTHREAD_FIELD_BYTE_OFFSET( "mythryl_handler_for_posix_signal_is_running",	mythryl_handler_for_posix_signal_is_running	);
    PRINT_PTHREAD_FIELD_BYTE_OFFSET( "all_posix_signals_seen_count",			all_posix_signals.seen_count			);
    PRINT_PTHREAD_FIELD_BYTE_OFFSET( "all_posix_signals_done_count",			all_posix_signals.done_count			);

    finish_generating_header_file( fd, unique_string );

    exit (0);
}



// COPYRIGHT (c) 1992 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

