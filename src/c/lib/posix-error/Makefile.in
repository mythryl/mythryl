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

LIBRARY =	libmythryl-posix-error.a

VERSION =	v-dummy

OBJS =	libmythryl-posix-error.o \
	posix-name-val.o \
	errmsg.o \
	geterror.o \
	listerrors.o \
	errno-sysconsts-table.o

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

