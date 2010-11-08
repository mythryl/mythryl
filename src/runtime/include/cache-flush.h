/* cache-flush.h
 *
 * System dependent includes and macros for flushing the cache.
 */

#ifndef _CACHE_FLUSH_
#define _CACHE_FLUSH_

#if defined(TARGET_X86)
/* 386 & 486 have unified caches and the pentium has hardware consistency */
#  define FlushICache(addr, size)

#elif ((defined(TARGET_RS6000) || defined(TARGET_PPC))&& defined(OPSYS_AIX))
#  include <sys/cache.h>
#  define FlushICache(addr, size)	_sync_cache_range((addr), (size))

#elif (defined(TARGET_SPARC) || defined(OPSYS_MKLINUX))
extern FlushICache (void *addr, int nbytes);


#elif (defined(TARGET_PPC) && (defined(OPSYS_LINUX) || defined(OPSYS_DARWIN) ))
extern FlushICache (void *addr, int nbytes);

#else
#  define FlushICache(addr, size)
#endif

#endif /* !_CACHE_FLUSH_ */



/* COPYRIGHT (c) 1994 AT&T Bell Laboratories
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */

