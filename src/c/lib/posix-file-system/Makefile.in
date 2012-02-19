#
# A template makefile for a C function library
#

SHELL =		/bin/sh

CLIB_DIR =	../
ROOT_DIR =	../..
INC_DIR =	$(ROOT_DIR)/h
OBJ_DIR =	$(ROOT_DIR)/o			# Generated .h files wind up here.
CONFIG_DIR =	$(ROOT_DIR)/config

INCLUDES =	-I$(INC_DIR) -I$(CLIB_DIR) -I$(OBJ_DIR)

MAKE =		make
AR =		ar
ARFLAGS =	rcv
CPP =		/lib/cpp
RANLIB =	ranlib

LIBRARY =	libmythryl-posix-file-system.a

VERSION =	v-dummy

OBJS =	libmythryl-posix-file-system.o \
        osval.o \
        chdir.o \
        getcwd.o \
        openf.o \
        mkstemp.o \
        umask.o \
        link.o \
        rename.o \
        symlink.o \
        mkdir.o \
        mkfifo.o \
        unlink.o \
        rmdir.o \
        readlink.o \
        stat.o \
        stat_64.o \
        access.o \
        chmod.o \
        fchmod.o \
        chown.o \
        fchown.o \
        ftruncate.o \
        ftruncate_64.o \
        utime.o \
        opendir.o \
        readdir.o \
        rewinddir.o \
        closedir.o \
        pathconf.o

$(LIBRARY)	: $(VERSION) $(OBJS)
	rm -rf $(LIBRARY)
	$(AR) $(ARFLAGS) $(LIBRARY) $(OBJS)
	$(RANLIB) $(LIBRARY)

$(VERSION) :
	echo "$(VERSION)" > $(VERSION)

.c.o:	$(INC_DIR)/system-dependent-unix-stuff.h $(INC_DIR)/runtime-base.h $(INC_DIR)/runtime-values.h \
	$(CLIB_DIR)/raise-error.h cfun-proto-list.h cfun-list.h
	$(CC) $(CFLAGS) $(DEFS) $(INCLUDES) -c $<

pathconf.o : ml_pathconf.h

ml_pathconf.h :
	VERSION=$(VERSION) CPP="$(CPP)" $(CONFIG_DIR)/generate-posix-names.sh _PC_ ml_pathconf.h

clean :
	@rm -f v-* *.o ml_pathconf.h $(LIBRARY)

