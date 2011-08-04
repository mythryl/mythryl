// posix-name-val.h
//
//
// Header file for handling string-to-int lookup.


#ifndef _LIB7_POSIX_NV_
#define _LIB7_POSIX_NV_

typedef struct {
  char*      name;
  int        val;
} name_val_t;

extern name_val_t *_lib7_posix_nv_lookup (char *, name_val_t *, int);

#endif // _LIB7_POSIX_NV_


// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

