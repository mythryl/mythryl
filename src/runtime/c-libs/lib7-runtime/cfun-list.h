/* cfun-list.h
 *
 *
 * This file lists the directory library of C functions that are callable by lib7.
 */

#ifndef CLIB_NAME
#define CLIB_NAME	"Lib7-Runtime"
#define CLIB_VERSION	"1.0"
#define CLIB_DATE	"December 15, 1994"
#endif

CFUNC("argv",			_lib7_Proc_argv,		"Void -> List String")
CFUNC("rawArgv",		_lib7_Proc_raw_argv,		"Void -> List String")
CFUNC("cmdName",		_lib7_Proc_cmd_name,		"Void -> String")
CFUNC("blastIn",		_lib7_runtime_blast_in,		"unt8_vector.Vector -> X")
CFUNC("blastOut",		_lib7_runtime_blast_out,	"X -> unt8_vector.Vector")
CFUNC("debug",			_lib7_runtime_debug,		"String -> Void")
CFUNC("dummy",			_lib7_runtime_dummy,		"String -> Void")
CFUNC("exportHeap",		_lib7_runtime_export_heap,	"String -> Bool")
CFUNC("spawn_to_disk",		_lib7_runtime_export_fun,	"(String * (List String -> Void)) -> Void")
CFUNC("gcControl",		_lib7_runtime_gc_ctl,		"List (String * Ref Int) -> Void")
CFUNC("intervalTick",		_lib7_runtime_itick,		"Void -> (Int, Int)")
CFUNC("allocCode",		_lib7_runtime_alloc_code,	"")
CFUNC("mkExec",			_lib7_runtime_mkexec,		"(Unt8_Vector, Int) -> Chunk -> Chunk")
CFUNC("mkLiterals",		_lib7_runtime_mkliterals,	"unt8_vector.Vector -> Ovec")
CFUNC("sysInfo",		_lib7_runtime_sysinfo,		"String -> Null_Or String")
CFUNC("record1",		_lib7_runtime_record1,		"Chunk -> Chunk")
CFUNC("recordMeld",		_lib7_runtime_recordmeld,	"(Chunk, Chunk) -> Chunk")
CFUNC("setIntervalTimer",	_lib7_runtime_setitimer,	"Null_Or (Int, Int) -> Null_Or (Int, Int)")



/* COPYRIGHT (c) 1994 AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
