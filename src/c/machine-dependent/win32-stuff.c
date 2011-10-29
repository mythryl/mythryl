// win32-stuff.c
//
// win32 specific utility code


#include "../mythryl-config.h"

#include <windows.h>
#include "system-dependent-stuff.h"
#include "runtime-base.h"

int GetPageSize()
{
  SYSTEM_INFO si;

  GetSystemInfo(&si);
  return (int) si.dwPageSize;
}

// COPYRIGHT (c) 1996 Bell Laboratories, Lucent Technologies.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.


