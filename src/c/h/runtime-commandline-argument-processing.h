// runtime-commandline-argument-processing.h
//
// Command-line argument processing.


#ifndef RUNTIME_COMMANDLINE_ARGUMENT_PROCESSING_H
#define RUNTIME_COMMANDLINE_ARGUMENT_PROCESSING_H



// Maximum length of option and
// argument parts of command-line options:
//
#define MAX_COMMANDLINE_ARGUMENT_PART_LENGTH	64

// Return TRUE iff option begins with "--runtime-":
//
extern Bool  is_runtime_option  (char* commandline_arg,  char* option,  char** arg);

extern int     get_size_option  (int scale,  char* size);



#endif //  RUNTIME_COMMANDLINE_ARGUMENT_PROCESSING_H



// COPYRIGHT (c) 1996 AT&T Research.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2013,
// released per terms of SMLNJ-COPYRIGHT.


