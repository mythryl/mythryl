/* tcsetattr.c
 *
 */

#include "../../config.h"

#include "runtime-unixdep.h"

#if HAVE_TERMIOS_H
#include <termios.h>
#endif

#include "runtime-base.h"
#include "runtime-values.h"
#include "runtime-heap.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"

/* _lib7_P_TTY_tcsetattr : int * int * termio_rep -> Void
 *    termio_rep = (word * word * word * word * String * word * word)
 *
 * Set parameters associated with tty.
 *
 * NOTE: the calls to cfset[io]speed by making the code more OS-dependent
 * and using the package of struct termios.
 */
lib7_val_t _lib7_P_TTY_tcsetattr (lib7_state_t *lib7_state, lib7_val_t arg)
{
    int              fd = REC_SELINT(arg, 0);
    int              action = REC_SELINT(arg, 1);
    lib7_val_t         termio_rep = REC_SEL(arg, 2);
    struct termios   data;

    data.c_iflag = REC_SELWORD(termio_rep, 0);
    data.c_oflag = REC_SELWORD(termio_rep, 1);
    data.c_cflag = REC_SELWORD(termio_rep, 2);
    data.c_lflag = REC_SELWORD(termio_rep, 3);

    memcpy (data.c_cc, GET_SEQ_DATAPTR(void, REC_SEL(termio_rep, 4)), NCCS);

    {   int status = cfsetispeed (&data, REC_SELWORD(termio_rep, 5));

	if (status < 0)
	    return RAISE_SYSERR(lib7_state, status);

	status = cfsetospeed (&data, REC_SELWORD(termio_rep, 6));

	if (status < 0) {
	    return RAISE_SYSERR(lib7_state, status);
        }
	status = tcsetattr(fd, action, &data);

	CHECK_RETURN_UNIT(lib7_state, status)
    }
}


/* COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
