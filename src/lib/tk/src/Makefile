# Makefile
#
# Token makefile added 2012-03-24 CrT just to suppress a
#
#     make[2]: *** No rule to make target `clean'.  Stop.
#
# message.  We get called by the   clean:   rule in
#
#     src/lib/tk/Makefile
#
# which file gets created by
#
#     sh/make-c-stuff
#
# from the template file
#
#     sh/_src_tk_makefile
#
# If we start using this makefile for anything fancier, it probably
# needs to become Makefile.in and then get processed via the usual
# automake/autoconfig logic.

MAKE =		make
AR =		ar
ARFLAGS =	rcv
RANLIB =	ranlib

clean:
	@rm -f *.o *~ *.log


