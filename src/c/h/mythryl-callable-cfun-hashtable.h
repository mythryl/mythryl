// mythryl-callable-cfun-hashtable.h
//
// API of   src/c/heapcleaner/mythryl-callable-cfun-hashtable.c

#ifndef MYTHRYL_CALLABLE_C_FNS_HASHTABLE_H
#define MYTHRYL_CALLABLE_C_FNS_HASHTABLE_H

typedef  struct heapfile_cfun_table  Heapfile_Cfun_Table;

// Our focal function here is to maintain a global hashtable
// of all C functions visible at the Mythryl level.
// We also track about a dozen assembly functions and
// half a dozen exceptions and refcells which are defined
// at the C level but visible at the Mythryl level.  We
// use "cfun" to refer to all of these.

// These are our core two functions:  One to
// enter a C function into the global hashtable
// one to subsequently retrieve it by name:
//
extern void          publish_cfun   (const char* name, Val addr);				// Make a C-level resource (almost always a function) visible at the Mythryl level.
extern Val           find_cfun      (const char* name);

// This function returns a C function name
// give its address.  It is used once, in
// a debug diagnostic function:
//
extern const char*   name_of_cfun    (Val addr);

// This function is used only during system
// maintenance, when renaming one of these
// C functions -- a process which is complicated
// by the way their names wind up embedded in
// the mythryl-runtime-intel32 executable and the various
// heapfiles:
//
extern void          publish_cfun2  (const char* name, const char* nickname, Val addr);		// Make a C resource (usually a function) visible at the Mythryl level, under two names. (Useful during maintenance, when renaming a symbol.)


// The remaining functions are used only when
// actually reading and writing heapfiles. Each
// heapfile contains a local table of just those
// C functions actually mentioned in the heapfile.

// Allocate and release heapfile-local tables:
//
extern Heapfile_Cfun_Table*   make_heapfile_cfun_table   ();
extern void                   free_heapfile_cfun_table   (Heapfile_Cfun_Table* table);

// Entry and retrieval of cfuns in heapfile-local tables:
//
extern Val                  add_cfun_to_heapfile_cfun_table   (Heapfile_Cfun_Table* table,  Val addr);
extern Val        get_cfun_address_from_heapfile_cfun_table   (Heapfile_Cfun_Table* table,  Val xref);
extern void   get_names_of_all_cfuns_in_heapfile_cfun_table   (Heapfile_Cfun_Table* table,  int* symbol_count, const char*** symbols);


// For the heapfile header we need to precompute how
// many bytes the heapfile cfun table will occupy on disk:
//
extern Punt   heapfile_cfun_table_bytesize   (Heapfile_Cfun_Table* table);

#endif // MYTHRYL_CALLABLE_C_FNS_HASHTABLE_H


// COPYRIGHT (c) 1992 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2012,
// released under Gnu Public Licence version 3.

