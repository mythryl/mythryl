// generate-system-signals.h-for-posix-systems.h


typedef struct {
    int   kernel_id_for_signal;					// Numeric value of SIGQUIT or such.
    char* signal_h__name_for_signal;				// The name #define'd for the signal in <signal.h>, e.g. "SIGQUIT".
    //								//
    char* mythryl_name_for_signal;				// The short name of the signal passed to Mythryl.
} Signal_Descriptor;

typedef struct {
    //
    Signal_Descriptor** sigs;					// An ordered vector of signal descriptions.
    //
    int			posix_signal_kinds;			// Number of posix interprocess signals supported on this system.
    int			lowest_valid_posix_signal_number;	// Minimum system signal number.
    int			highest_valid_posix_signal_number;	// Maximum system signal number.
    //
} Runtime_System_Signal_Table;

extern Runtime_System_Signal_Table*  sort_runtime_system_signal_table   ();				// sort_runtime_system_signal_table	def in   src/c/config/posix-signals.c



// COPYRIGHT (c) 1995 AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2012,
// released under Gnu Public Licence version 3.

