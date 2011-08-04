// generate-system-signals.h-for-posix-systems.h


typedef struct {
    int   sig;							// The POSIX signal code.
    char* sigName;						// The symbolic name of the signal (i.e., the #define name.
    //								//
    char* shortName;						// The short name of the signal passed to Mythryl.
} Signal_Descriptor;

typedef struct {
    //
    Signal_Descriptor** sigs;					// An ordered vector of signal descriptions.
    //
    int			posix_signal_kinds;			// Number of posix interprocess signals supported on this system.
    int			runtime_generated_signal_kinds;		// Number of different runtime-generated signals.
    int			lowest_valid_posix_signal_number;	// Minimum system signal number.
    int			highest_valid_posix_signal_number;	// Maximum system signal number.
    //
} Runtime_System_Signal_Table;

extern Runtime_System_Signal_Table*  sort_runtime_system_signal_table   ();				// sort_runtime_system_signal_table	def in   src/c/config/posix-signals.c



// COPYRIGHT (c) 1995 AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

