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
#include "../raise-error.h"

/* _lib7_OpenCV_cvLoadImage : String -> Image
 *
 */
Val

_lib7_OpenCV_cvLoadImage (Task *task, Val arg)
{
#if HAVE_OPENCV_CV_H && HAVE_LIBCV

    char*      filename  =  HEAP_STRING_AS_C_STRING( arg );				// Last use of 'arg'.
    IplImage*  ipl_image =  cvLoadImage( filename, CV_LOAD_IMAGE_UNCHANGED );

    if (!ipl_image)   RAISE_ERROR__MAY_HEAPCLEAN(task, "cvLoadImage returned NULL", NULL);

    {   
	// Copy image into heap, so that it can be
	// garbage-collected when no longer needed:
	//
        Val header_data  =  make_biwordslots_vector_sized_in_bytes__may_heapclean(  task, ipl_image, sizeof(IplImage), NULL);				Roots roots1 = { &header_data,   NULL };
        Val header       =  make_vector_header(					    task, UNT8_RO_VECTOR_TAGWORD, header_data, sizeof(IplImage));	Roots roots2 = { &header,	&roots1 };

//	c_roots__global[c_roots_count__global++] = &header;			// Protect header from garbage collection while allocating image.  (Obsoleted by 'roots' mechanism.)

	Val image_data   =  make_biwordslots_vector_sized_in_bytes__may_heapclean(  task,  ipl_image->imageData, ipl_image->imageSize, &roots2);
        Val image        =  make_vector_header(               			    task,  UNT8_RO_VECTOR_TAGWORD, image_data, ipl_image->imageSize);
        
//	--c_roots_count__global;
	cvReleaseImage( &ipl_image );

	return  make_two_slot_record( task, header, image );
    }

#else

    extern char* no_opencv_support_in_runtime;
    //
    return  RAISE_ERROR__MAY_HEAPCLEAN(task, no_opencv_support_in_runtime, NULL);

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
