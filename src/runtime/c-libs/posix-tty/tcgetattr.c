/* tcgetattr.c
 *
 */

#include "../../config.h"

#include "runtime-unixdep.h"
#include INCLUDE_TIME_H

#if HAVE_TERMIOS_H
#include <termios.h>
#endif

#include "runtime-base.h"
#include "runtime-values.h"
#include "runtime-heap.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"

/* _lib7_P_TTY_tcgetattr : int -> (word * word * word * word * String * word * word)
 *
 * Get parameters associated with tty.
 *
 * NOTE: the calls to cfget[io] speed by making the code more OS-dependent
 * and using the package of struct termios.
 */
lib7_val_t _lib7_P_TTY_tcgetattr (lib7_state_t *lib7_state, lib7_val_t arg)
{
    int             fd = INT_LIB7toC(arg);
    lib7_val_t      iflag, oflag, cflag, lflag;
    lib7_val_t      cc, ispeed, ospeed, chunk;
    struct termios  data;

    int status = tcgetattr(fd, &data);

    if (status < 0) {
        return RAISE_SYSERR(lib7_state, status);
    }

    WORD_ALLOC (lib7_state, iflag, data.c_iflag);
    WORD_ALLOC (lib7_state, oflag, data.c_oflag);
    WORD_ALLOC (lib7_state, cflag, data.c_cflag);
    WORD_ALLOC (lib7_state, lflag, data.c_lflag);
    WORD_ALLOC (lib7_state, ispeed, cfgetispeed (&data));
    WORD_ALLOC (lib7_state, ospeed, cfgetospeed (&data));
    
    /* Allocate the vector.
     * Note that this might cause a GC:
     */
    cc = LIB7_AllocString (lib7_state, NCCS);

    memcpy (GET_SEQ_DATAPTR(void, cc), data.c_cc, NCCS);

    LIB7_AllocWrite (lib7_state, 0, MAKE_DESC(DTAG_record, 7));
    LIB7_AllocWrite (lib7_state, 1, iflag);
    LIB7_AllocWrite (lib7_state, 2, oflag);
    LIB7_AllocWrite (lib7_state, 3, cflag);
    LIB7_AllocWrite (lib7_state, 4, lflag);
    LIB7_AllocWrite (lib7_state, 5, cc);
    LIB7_AllocWrite (lib7_state, 6, ispeed);
    LIB7_AllocWrite (lib7_state, 7, ospeed);
    chunk = LIB7_Alloc(lib7_state, 7);

    return chunk;
}


/* COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
