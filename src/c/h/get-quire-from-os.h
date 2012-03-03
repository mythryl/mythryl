// get-quire-from-os.h
//
// An OS-independent view of memory.
//
// This supports allocation of memory chunks aligned
// to BOOK_BYTESIZE byte boundaries -- see								// BOOK_BYTESIZE is typically 64KB.
//
//     src/c/h/sibid.h

// 'quire' (pronounced like choir) designates a multipage ram region
//         allocated directly from the host operating system.
//
//     quire: 2. A collection of leaves of parchment or paper,
//               folded one within the other, in a manuscript or book.
//                     -- http://www.thefreedictionary.com/quire

#ifndef OBTAIN_QUIRE_FROM_OS_H
#define OBTAIN_QUIRE_FROM_OS_H

// Quire_Prefix
//
// Define access to the first two fields of
//     struct quire
// The fields in this struct are system-dependent,
// but these two fields are the same in all versions.
//
typedef struct {
    //
    Punt	base;											// Base address of the region.
    Punt	bytesize;										// Region size in bytes
    //
} Quire_Prefix;												// This type is referenced only in the following two macros.
//
#define BASE_ADDRESS_OF_QUIRE(region)	(((Quire_Prefix*)(region))->base)
#define     BYTESIZE_OF_QUIRE(region)	(((Quire_Prefix*)(region))->bytesize)

typedef   struct quire   Quire;										// Depending on host OS, struct quire def actually in use will be one of:
													// struct quire				def in   src/c/ram/get-quire-from-mmap.c
													// struct quire				def in   src/c/ram/get-quire-from-win32.c
													// struct quire				def in   src/c/ram/get-quire-from-mach.c

// This API defines three client-accessible functions:
//
extern Quire*   obtain_quire_from_os   (Vunt  bytesize);					//     obtain_quire_from_os		def in   src/c/ram/get-quire-from-os-stuff.c
extern void		       return_quire_to_os     (Quire*  region);					//     return_quire_to_os		def in   src/c/ram/get-quire-from-os-stuff.c

extern void                    set_up_quire_os_interface        (void);					// set_up_quire_os_interface		def in   src/c/ram/get-quire-from-mach.c
													// set_up_quire_os_interface		def in   src/c/ram/get-quire-from-mmap.c
													// set_up_quire_os_interface		def in   src/c/ram/get-quire-from-win32.c


#endif // OBTAIN_QUIRE_FROM_OS_H



// COPYRIGHT (c) 1992 AT&T Bell Laboratories
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.


