// writer.h
//
// This is an abstraction of a buffered output
// device for writing heap data.


#ifndef _WRITER_
#define _WRITER_

#include <stdio.h>  /* for FILE */

// Writer
//
typedef struct writer {
    //
    Bool	seen_error;
    void*	data;
    void	(*put_word)(struct writer*, Vunt);
    void	(*write)(struct writer*, const void *, Vunt);
    void	(*flush)(struct writer*);
    long	(*tell)(struct writer*);
    void	(*seek)(struct writer*, long offset);
    void	(*free)(struct writer*);
    //
} Writer;

// Open a file for writing, and make a file for it
//
extern Writer* WR_OpenFile (FILE* file);

// Make a writer from a region of memory:
//
extern Writer* WR_OpenMem  (Unt8* data,  Vunt len);

#define WR_ERROR(wr)			((wr)->seen_error)
#define WR_PUT(wr, w)			((wr)->put_word((wr), (w)))
#define WR_WRITE(wr, data, nbytes)	((wr)->write((wr), (data), (nbytes)))
#define WR_FLUSH(wr)			((wr)->flush(wr))
#define WR_TELL(wr)			((wr)->tell(wr))
#define WR_SEEK(wr, offset)		((wr)->seek((wr), (offset)))
#define WR_FREE(wr)			((wr)->free(wr))

#endif // _WRITER_


// COPYRIGHT (c) 1995 AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2014,
// released per terms of SMLNJ-COPYRIGHT.


