// cvRandInt.c

#include "../../mythryl-config.h"

#if HAVE_OPENCV_CV_H
#include <opencv/cv.h>
#endif

#include <stdio.h>
#include <stdlib.h>

#include "runtime-base.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "cfun-proto-list.h"
#include "../raise-error.h"

/* _lib7_OpenCV_cvRandInt : Random_Number_Generator -> Int
 *
 */
Val

_lib7_OpenCV_cvRandInt (Task *task, Val arg)
{
#if HAVE_OPENCV_CV_H && HAVE_LIBCV

    CvRNG* rng
        =
        GET_VECTOR_DATACHUNK_AS( CvRNG*, arg);

    int result
        =
        cvRandInt( rng );

    return TAGGED_INT_FROM_C_INT( result );

#else

    extern char* no_opencv_support_in_runtime;

    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_opencv_support_in_runtime, NULL);

#endif
}

// Code by Jeff Prothero: Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.
