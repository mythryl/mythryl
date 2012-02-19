#
# A template makefile for a C function library
#

SHELL =		/bin/sh

ROOT_DIR =	../..
INC_DIR =	$(ROOT_DIR)/h
CONFIG_DIR =	$(ROOT_DIR)/config
CLIB_DIR =	../

INCLUDES =	-I$(INC_DIR) -I$(CLIB_DIR) -I../../o

MAKE =		make
AR =		ar
ARFLAGS =	rcv
CPP =		/lib/cpp
RANLIB =	ranlib

LIBRARY =	libmythryl-posix-process-environment.a

VERSION =	v-dummy

OBJS =	libmythryl-posix-process-environment.o \
        getpid.o \
        getppid.o \
        getuid.o \
        geteuid.o \
        getgid.o \
        getegid.o \
        setuid.o \
        setgid.o \
        getgroups.o \
        getlogin.o \
        getpgrp.o \
        setsid.o \
        setpgid.o \
        uname.o \
        time.o \
        times.o \
        getenv.o \
        environ.o \
        ctermid.o \
        ttyname.o \
        sysconf.o \
        isatty.o

$(LIBRARY)	: $(VERSION) $(OBJS)
	rm -rf $(LIBRARY)
	$(AR) $(ARFLAGS) $(LIBRARY) $(OBJS)
	$(RANLIB) $(LIBRARY)

$(VERSION) :
	echo "$(VERSION)" > $(VERSION)

.c.o:	$(INC_DIR)/system-dependent-unix-stuff.h $(INC_DIR)/runtime-base.h $(INC_DIR)/runtime-values.h \
	$(CLIB_DIR)/raise-error.h cfun-proto-list.h cfun-list.h
	$(CC) $(CFLAGS) $(DEFS) $(INCLUDES) -c $<

sysconf.o : ml_sysconf.h

ml_sysconf.h :
	VERSION=$(VERSION) CPP="$(CPP)" $(CONFIG_DIR)/generate-posix-names.sh _SC_ ml_sysconf.h

clean :
	@rm -f v-* *.o ml_sysconf.h $(LIBRARY)

