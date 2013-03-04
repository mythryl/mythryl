// cutil.c
//
// Some useful user-level C functions.
// Declared and registered on the C side by cutil-cfuns.h

#include "../../mythryl-config.h"

char *ptos(void *p)
{
#ifdef DEBUG_C_CALLS
    printf("in ptos with string \"%s\"",(char *)p);	fflush(stdout);
#endif
    return (char *) p;
}

int ptoi(int *p)
{
    int i;

    // p may not be pointing to an aligned int, hence the memcpy:
    //
    memcpy (&i, p, sizeof(int));

    return i;
}


// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2012,
// released per terms of SMLNJ-COPYRIGHT.


