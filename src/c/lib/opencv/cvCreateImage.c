// cvCreateImage.c

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

/* _lib7_OpenCV_cvCreateImage : String -> Image
 *
 */
Val

_lib7_OpenCV_cvCreateImage (Task *task, Val arg)
{
#if HAVE_OPENCV_CV_H && HAVE_LIBCV
  /*
    int height =  700;
    int width  = 1000;
    cvCreateImage( cvSize(1000,700),8,3);
    return HEAP_VOID;
  */

    IplImage img;

    Val data   =  make_int64_vector_sized_in_bytes(  task, &img, sizeof(img));

    Val result;    SEQHDR_ALLOC(task, result, UNT8_RO_VECTOR_TAGWORD, data, sizeof(img));

    return result;

#else

    extern char* no_opencv_support_in_runtime;
    return RAISE_ERROR(task, no_opencv_support_in_runtime);

#endif
}



/* Notes:

   IplImage      is defined in		cxcore/include/cxtypes.h

   cvCreateImage is defined in		cxcore/src/cxarray.cpp



 */

// Code by Jeff Prothero: Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.


