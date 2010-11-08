/* lseek_64.c
 *
 *   Like lseek.c, but with 64-bit position values.
 *
 */

#include "../../config.h"

#include "runtime-unixdep.h"

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#include "runtime-base.h"
#include "runtime-values.h"
#include "runtime-heap.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"

/* _lib7_P_IO_lseek_64 : int * word32 * word32 * int -> word32 * word32
 *
 * Move read/write file pointer.
 */
lib7_val_t _lib7_P_IO_lseek_64 (lib7_state_t *lib7_state, lib7_val_t arg)
{
    int         fd = REC_SELINT(arg, 0);
    off_t       offset =
      (sizeof(off_t) > 4)
      ? (((off_t)WORD_LIB7toC(REC_SEL(arg, 1))) << 32) |
        ((off_t)(WORD_LIB7toC(REC_SEL(arg, 2))))
      : ((off_t)(WORD_LIB7toC(REC_SEL(arg, 2))));
    off_t       pos;
    int         whence = REC_SELINT(arg, 3);
    lib7_val_t    poshi, poslo, chunk;

    pos = lseek(fd, offset, whence);

    if (pos < 0)
        RAISE_SYSERR (lib7_state, (int)pos);

    if (sizeof(off_t) > 4) {
      WORD_ALLOC (lib7_state, poshi, (Unsigned32_t) (pos >> 32));
    } else {
      WORD_ALLOC (lib7_state, poshi, (Unsigned32_t) 0);
    }

    WORD_ALLOC (lib7_state, poslo, (Unsigned32_t) pos);

    REC_ALLOC2 (lib7_state, chunk, poshi, poslo);

    return chunk;
} /* end of _lib7_P_IO_lseek_64 */


/* Copyright (c) 2004 by The Fellowship of SML/NJ
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
