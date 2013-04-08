#
# makefile for the C library that provides Lib7-callable C functions
#

SHELL =		/bin/sh

INC_DIR =	../../h
CLIB_DIR =	../

INCLUDES =	-I$(INC_DIR) -I$(CLIB_DIR) -I../../o

MAKE =		make
AR =		ar
ARFLAGS =	rcv
RANLIB =	ranlib

LIBRARY =	libmythryl-ccalls.a

VERSION =	v-dummy



OBJS =		libmythryl-ccalls.o \
		ccalls.o \
		ccalls-fns.o \
		c-entry.o \
		cutil.o

C_CFUNS = cutil-cfuns.h

$(LIBRARY)	: $(VERSION) $(OBJS)
	rm -rf $(LIBRARY)
	$(AR) $(ARFLAGS) $(LIBRARY) $(OBJS)
	$(RANLIB) $(LIBRARY)

$(VERSION) :
	echo "$(VERSION)" > $(VERSION)

libmythryl-ccalls.o: $(INC_DIR)/system-dependent-stuff.h $(INC_DIR)/runtime-base.h \
	$(INC_DIR)/runtime-values.h \
	$(CLIB_DIR)/raise-error.h cfun-proto-list.h cfun-list.h $(C_CFUNS)
	$(CC) $(CFLAGS) $(DEFS) $(INCLUDES) $(LOCAL_INCLUDES) -c libmythryl-ccalls.c

c-entry.o:	c-entry.asm ccalls.h
	$(CPP) -D_ASM_ $(DEFS) $(INCLUDES) c-entry.asm > c-entry.s
	$(AS) -o c-entry.o c-entry.s

.c.o:	$(INC_DIR)/system-dependent-stuff.h $(INC_DIR)/runtime-base.h $(INC_DIR)/runtime-values.h \
	$(CLIB_DIR)/raise-error.h \
	$(INC_DIR)/flush-instruction-cache-system-dependent.h \
	ccalls.h \
	cfun-proto-list.h cfun-list.h
	$(CC) $(CFLAGS) $(DEFS) $(INCLUDES) $(LOCAL_INCLUDES) -c $<

clean :
	@rm -f v-* *.o *.s $(LIBRARY)

