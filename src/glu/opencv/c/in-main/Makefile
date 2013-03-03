#
# Makefile for the Opencv (Opencv) library glue layer
#
# We get invoked by:
#     src/c/o/Makefile.in

SHELL =		/bin/sh

INC_DIR =	../../../../c/h
CLIB_DIR =	../../../../c/lib

INCLUDE =	-I$(INC_DIR) -I$(CLIB_DIR) -I../../../../c/o

AR =		ar
ARFLAGS =	rcv
RANLIB =	ranlib

LIBRARY =	libmythryl-opencv.a

VERSION =	v-dummy

OBJS =		libmythryl-opencv.o


$(LIBRARY)	: $(VERSION) $(OBJS)
	rm -rf $(LIBRARY)
	$(AR) $(ARFLAGS) $(LIBRARY) $(OBJS)
	$(RANLIB) $(LIBRARY)

$(VERSION) :
	echo "$(VERSION)" > $(VERSION)

.c.o:	$(INC_DIR)/system-dependent-unix-stuff.h $(INC_DIR)/runtime-base.h $(INC_DIR)/runtime-values.h \
	$(CLIB_DIR)/raise-error.h cfun-proto-list.h cfun-list.h
	$(CC) $(CFLAGS) $(DEFS) $(INCLUDE) -c $<

# This gets invoked mainly via
#     clean:
# target in src/c/lib/Makefile
#
clean :
	@rm -f v-* *.o $(LIBRARY)

