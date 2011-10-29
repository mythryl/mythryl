// cvLoadImage.c

#include "../../mythryl-config.h"

#if HAVE_OPENCV_CV_H
#include <opencv/cv.h>
#endif

#if HAVE_OPENCV_HIGHGUI_H
#include <opencv/highgui.h>
#endif

#include <stdio.h>
#include <stdlib.h>

#include "runtime-base.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "runtime-globals.h"
#include "cfun-proto-list.h"
#include "../lib7-c.h"

/* _lib7_OpenCV_cvLoadImage : String -> Image
 *
 */
Val

_lib7_OpenCV_cvLoadImage (Task *task, Val arg)
{
#if HAVE_OPENCV_CV_H && HAVE_LIBCV

    char*      filename  =  HEAP_STRING_AS_C_STRING( arg );
    IplImage*  ipl_image =  cvLoadImage( filename, CV_LOAD_IMAGE_UNCHANGED );

    if (!ipl_image)   RAISE_ERROR(task, "cvLoadImage returned NULL");

    {   
	// Copy image into heap, so that it can be
	// garbage-collected when no longer needed:
	//
	Val header;	Val header_data;
	Val image;	Val  image_data;

	Val result;

        header_data  =  make_int2_vector_sized_in_bytes(  task, ipl_image, sizeof(IplImage));
        SEQHDR_ALLOC(task, header, UNT8_RO_VECTOR_TAGWORD, header_data, sizeof(IplImage));

	c_roots_global[c_roots_count_global++] = &header;			// Protect header from garbage collection while allocating image.

	image_data   =  make_int2_vector_sized_in_bytes(  task, ipl_image->imageData, ipl_image->imageSize);
        SEQHDR_ALLOC(task, image, UNT8_RO_VECTOR_TAGWORD, image_data, ipl_image->imageSize);
        
	--c_roots_count_global;
	cvReleaseImage( &ipl_image );

	REC_ALLOC2(task, result, header, image);
	return           result;
    }

#else

    extern char* no_opencv_support_in_runtime;
    return RAISE_ERROR(task, no_opencv_support_in_runtime);

#endif
}



// Notes:
//
//   IplImage       is defined in		cxcore/include/cxtypes.h
//
//   cvCreateImage  is defined in		cxcore/src/cxarray.cpp
//   cvCreateData   is defined in		cxcore/src/cxarray.cpp
//   cvReleaseImage is defined in		cxcore/src/cxarray.cpp
//
//   cvLoadImage    is defined in		otherlibs/highgui/loadsave.cpp
//
//
//

// Code by Jeff Prothero: Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.
