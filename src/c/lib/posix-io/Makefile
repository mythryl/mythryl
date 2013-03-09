#
# A template makefile for a C function library
#

SHELL =		/bin/sh

INC_DIR =	../../h
CLIB_DIR =	../

INCLUDES =	-I$(INC_DIR) -I$(CLIB_DIR) -I../../o

MAKE =		make
AR =		ar
ARFLAGS =	rcv
RANLIB =	ranlib

LIBRARY =	libmythryl-posix-io.a

VERSION =	v-dummy

OBJS =	libmythryl-posix-io.o \
        osval.o \
        pipe.o \
        close.o \
        copy.o \
        dup.o \
        dup2.o \
        equal.o \
        read.o \
        readbuf.o \
        write.o \
        writebuf.o \
        fcntl_d.o \
        fcntl_gfd.o \
        fcntl_sfd.o \
        fcntl_gfl.o \
        fcntl_sfl.o \
        fcntl_l.o \
        fcntl_l_64.o \
        lseek.o \
        lseek_64.o \
        fsync.o

$(LIBRARY)	: $(VERSION) $(OBJS)
	rm -rf $(LIBRARY)
	$(AR) $(ARFLAGS) $(LIBRARY) $(OBJS)
	$(RANLIB) $(LIBRARY)

$(VERSION) :
	echo "$(VERSION)" > $(VERSION)

.c.o:	$(INC_DIR)/system-dependent-unix-stuff.h $(INC_DIR)/runtime-base.h $(INC_DIR)/runtime-values.h \
	$(CLIB_DIR)/raise-error.h cfun-proto-list.h cfun-list.h
	$(CC) $(CFLAGS) $(DEFS) $(INCLUDES) -c $<

clean :
	@rm -f v-* *.o $(LIBRARY)

