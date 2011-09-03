// cvRNG.c

#include "../../config.h"

#if HAVE_OPENCV_CV_H
#include <opencv/cv.h>
#endif

#include <stdio.h>
#include <stdlib.h>

#include "runtime-base.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "cfun-proto-list.h"
#include "../lib7-c.h"

#include <string.h>	// XXX BUGGO DELETEME

/* _lib7_OpenCV_cvRNG : Int -> Random_Number_Generator
 *
 */
Val

_lib7_OpenCV_cvRNG (Task *task, Val arg)
{

#if HAVE_OPENCV_CV_H && HAVE_LIBCV

    int init = TAGGED_INT_TO_C_INT( arg );
    CvRNG rng = cvRNG( (unsigned) init);

    Val data   =  make_int2_vector_sized_in_bytes( task, &rng, sizeof(rng) );

    Val result;    SEQHDR_ALLOC(task, result, UNT8_RO_VECTOR_TAGWORD, data, sizeof(rng));

    return result;

#else

    extern char* no_opencv_support_in_runtime;

    return RAISE_ERROR(task, no_opencv_support_in_runtime);

#endif
}

// Code by Jeff Prothero: Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

