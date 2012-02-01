#
# The makefile for the memory management library.  This get recursively
# invoked with the OBJS variable set to the appropriate value.
#

MAKE =		make
AR =		ar
ARFLAGS =	rcv
RANLIB =	ranlib

VERSION =	v-dummy

LIB =		libmythryl-ram.a

MMAP_OBJS =	get-quire-from-mmap.o

MACH_OBJS =	get-quire-from-mach.o

BRK_OBJS =	mem-brk.o malloc.o

$(LIB) :	$(VERSION) $(OBJS)
	$(AR) $(ARFLAGS) $(LIB) $(OBJS)
	$(RANLIB) $(LIB)

$(VERSION) :
	echo "$(VERSION)" > $(VERSION)

get-quire-from-mmap.o get-quire-from-mach.o mem-brk.o:	get-quire-from-os-stuff.c

.o:	../h/runtime-base.h ../h/system-dependent-unix-stuff.h ../h/sibid.h

.c.o:
	$(CC) $(CFLAGS) $(DEFS) $(INCLUDES) -c $<

clean :
	@rm -f v-* *.o $(LIB)

