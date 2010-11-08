/* cvRandInt.c
 *
 */

#include "../../config.h"

#if HAVE_OPENCV_CV_H
#include <opencv/cv.h>
#endif

#include <stdio.h>
#include <stdlib.h>

#include "runtime-base.h"
#include "runtime-values.h"
#include "runtime-heap.h"
#include "cfun-proto-list.h"
#include "../lib7-c.h"

/* _lib7_OpenCV_cvRandInt : Random_Number_Generator -> Int
 *
 */
lib7_val_t

_lib7_OpenCV_cvRandInt (lib7_state_t *lib7_state, lib7_val_t arg)
{
#if HAVE_OPENCV_CV_H && HAVE_LIBCV

    CvRNG* rng
        =
        GET_SEQ_DATAPTR( CvRNG, arg);

    int result
        =
        cvRandInt( rng );

    return INT_CtoLib7( result );

#else

    extern char* no_opencv_support_in_runtime;
    return RAISE_ERROR(lib7_state, no_opencv_support_in_runtime);

#endif
}

/* Code by Jeff Prothero: Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
