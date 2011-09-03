// runtime-timer.h


#ifndef RUNTIME_TIMER_H
#define RUNTIME_TIMER_H

#include "runtime-base.h"

// We define our own type to represent time values,
// since some systems have
//     struct timeval
// but others do not.
//
typedef struct {
    Int1	seconds;
    Int1	uSeconds;
} Time;

extern void  get_cpu_time             (Time* user_t,  Time* sys_t);
extern void  start_cleaning_timer (Pthread* pthread);
extern void  stop_cleaning_timer  (Pthread* pthread, long* time);

#endif // RUNTIME_TIMER_H


// COPYRIGHT (c) 1992 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

