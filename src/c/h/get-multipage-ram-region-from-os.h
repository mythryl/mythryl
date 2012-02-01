// get-multipage-ram-region-from-os.h
//
// An OS-independent view of memory.
//
// This supports allocation of memory chunks aligned
// to BOOK_BYTESIZE byte boundaries -- see
//
//     src/c/h/sibid.h

// 'quire' (pronounced like choir) designates a multipage ram region
//         allocated directly from the host operating system.
//
//     quire: 2. A collection of leaves of parchment or paper,
//               folded one within the other, in a manuscript or book.
//                     -- http://www.thefreedictionary.com/quire

#ifndef OBTAIN_MULTIPAGE_RAM_REGION_FROM_OS_H
#define OBTAIN_MULTIPAGE_RAM_REGION_FROM_OS_H

// Multipage_Ram_Region_Prefix
//
// Define access to the first two fields of
//     struct multipage_ram_region
// The fields in this struct are system-dependent,
// but these two fields are the same in all versions.
//
typedef struct {
    //
    Punt	base;											// Base address of the region.
    Punt	bytesize;										// Region size in bytes
    //
} Multipage_Ram_Region_Prefix;											// This type is referenced only in the following two macros.
//
#define BASE_ADDRESS_OF_MULTIPAGE_RAM_REGION(region)	(((Multipage_Ram_Region_Prefix*)(region))->base)
#define     BYTESIZE_OF_MULTIPAGE_RAM_REGION(region)	(((Multipage_Ram_Region_Prefix*)(region))->bytesize)

typedef   struct multipage_ram_region   Multipage_Ram_Region;


// This API defines three client-accessible functions:
//
extern Multipage_Ram_Region*   obtain_multipage_ram_region_from_os   (Val_Sized_Unt  bytesize);		//     obtain_multipage_ram_region_from_os		def in   src/c/ram/get-multipage-ram-region-from-os-stuff.c
extern void		       return_multipage_ram_region_to_os     (Multipage_Ram_Region*  region);		//     return_multipage_ram_region_to_os		def in   src/c/ram/get-multipage-ram-region-from-os-stuff.c

extern void                    set_up_multipage_ram_region_os_interface        (void);				// set_up_multipage_ram_region_os_interface		def in   src/c/ram/get-multipage-ram-region-from-mach.c
														// set_up_multipage_ram_region_os_interface		def in   src/c/ram/get-multipage-ram-region-from-mmap.c
														// set_up_multipage_ram_region_os_interface		def in   src/c/ram/get-multipage-ram-region-from-win32.c


#endif // OBTAIN_MULTIPAGE_RAM_REGION_FROM_OS_H



// COPYRIGHT (c) 1992 AT&T Bell Laboratories
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.


