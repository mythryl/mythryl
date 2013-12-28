// flush-instruction-cache-system-dependent.h
//
// System-dependent includes and macros for flushing the cache.


#ifndef CACHE_FLUSH_H
#define CACHE_FLUSH_H

#if defined(TARGET_INTEL32)
    // 386 & 486 have unified caches and the pentium has hardware consistency:
    //
    # define flush_instruction_cache(addr, size)

#elif ((defined(TARGET_PWRPC32)) && defined(OPSYS_AIX))

    #include <sys/cache.h>
    #define flush_instruction_cache(addr, size)	_sync_cache_range((addr), (size))

#elif (defined(TARGET_SPARC32) || defined(OPSYS_MKLINUX))

    extern flush_instruction_cache (void *addr, int nbytes);

#elif (defined(TARGET_PWRPC32) && (defined(OPSYS_LINUX) || defined(OPSYS_DARWIN) ))

    extern flush_instruction_cache (void *addr, int nbytes);

#else
    #error No instruction-cache flush defined for this target architecture.
#endif

#endif   // CACHE_FLUSH_H



// COPYRIGHT (c) 1994 AT&T Bell Laboratories
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2014,
// released per terms of SMLNJ-COPYRIGHT.


