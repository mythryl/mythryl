// cfun-list.h
//
// This file lists the "heap" library of Mythryl-callable
// C functions, accessible at the Mythryl level via:
//
//     my  get_program_name:  Void -> String
//         =
//	   mythryl_callable_c_library_interface::find_c_function { lib_name => "heap", fun_name => "program_name_from_commandline" };
// 
// or such.
// 
// We get #included by both:
//
//     src/c/lib/heap/libmythryl-heap.c
//     src/c/lib/heap/cfun-proto-list.h
//
// This table ultimately gets searched by
//
//     get_mythryl_callable_c_function() 	in   src/c/lib/mythryl-callable-c-libraries.c

#ifndef CLIB_NAME
#define CLIB_NAME	"heap"
#define CLIB_VERSION	"1.0"
#define CLIB_DATE	"December 15, 1994"
#endif

CFUNC("commandline_args","commandline_args",		_lib7_proc_commandline_args,		"Void -> List String")
CFUNC("raw_commandline_args","raw_commandline_args",	_lib7_proc_raw_commandline_args,		"Void -> List String")
CFUNC("program_name_from_commandline","program_name_from_commandline",	_lib7_proc_program_name_from_commandline,		"Void -> String")
CFUNC("unpickle_datastructure","unpickle_datastructure",_lib7_runtime_unpickle_datastructure,		"unt8_vector.Vector -> X")
CFUNC("pickle_datastructure","pickle_datastructure",	_lib7_runtime_pickle_datastructure,	"X -> unt8_vector.Vector")
CFUNC("debug","debug",					_lib7_runtime_debug,		"String -> Void")
CFUNC("dummy","dummy",					_lib7_runtime_dummy,		"String -> Void")
CFUNC("export_heap","export_heap",		_lib7_runtime_export_heap,	"String -> Bool")
CFUNC("spawn_to_disk","spawn_to_disk",		_lib7_runtime_export_fun,	"(String, (List(String) -> Void)) -> Void")
CFUNC("cleaner_control","heapcleaner_control",		_lib7_cleaner_control,		"List (String * Ref Int) -> Void")
CFUNC("interval_tick__unimplemented","interval_tick__unimplemented",		_lib7_runtime_interval_tick__unimplemented,	"Void -> (Int, Int)")				// Currently UNIMPLEMENTED
CFUNC("allocate_codechunk","allocate_codechunk",		_lib7_runtime_allocate_codechunk,	"Int -> rw_unt8_vector::Rw_Vector")
CFUNC("make_codechunk_executable","make_codechunk_executable",			_lib7_runtime_make_codechunk_executable,		"(Unt8_Vector, Int) -> Chunk -> Chunk")
CFUNC("make_package_literals_via_bytecode_interpreter","make_package_literals_via_bytecode_interpreter",		_lib7_runtime_make_package_literals_via_bytecode_interpreter,	"unt8_vector::Vector -> Ovec")
CFUNC("get_platform_property","get_platform_property",		_lib7_runtime_get_platform_property,		"String -> Null_Or String")
CFUNC("make_single_slot_tuple","make_single_slot_tuple",	_lib7_runtime_make_single_slot_tuple,		"Chunk -> Chunk")
CFUNC("concatenate_two_tuples","concatenate_two_tuples",		_lib7_runtime_concatenate_two_tuples,	"(Chunk, Chunk) -> Chunk")
CFUNC("set_sigalrm_frequency","set_sigalrm_frequency",	_lib7_runtime_set_sigalrm_frequency,	"Null_Or (Int, Int) -> Null_Or (Int, Int)")



// COPYRIGHT (c) 1994 AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

