# This is the main makefile for the Mythryl runtime system
#
# Currently it will get invoked via the relevant platform-specific
# makefile, which in practice usually means
#
#     src/c/o/Makefile.intel32-linux
#
# which in turn gets invoked via the
#
#     $MAKE -j $CORES -f Makefile.$ARCHITECTURE-$OS $EXTRA_DEFS
#
# line in
#   
#     sh/make-c-stuff
#
# in response to a toplevel "make compiler" at the Linux commandline.


SHELL :=	/bin/sh
CC :=		gcc
CPP :=		gcc -E
LD_LIBS2 :=	$(LD_LIBS) -ldisasm -ldl -lm 
AS :=		as
AR :=		ar
ARFLAGS :=	rcv
RANLIB :=	ranlib

# Dummy version:
#
VERSION :=	v-dummy

# Target executable:
#
RUNTIME :=	interactive-mythryl-runtime-intel32

# Source directories:
#
RUNTIME_DIR :=	..
OBJS_DIR :=		$(RUNTIME_DIR)/o
HEAPCLEANER_DIR :=	$(RUNTIME_DIR)/heapcleaner
#
H_DIR :=		$(RUNTIME_DIR)/h
MAIN_DIR :=		$(RUNTIME_DIR)/main
MACH_DIR :=		$(RUNTIME_DIR)/machine-dependent
#
RAM_DIR :=		$(RUNTIME_DIR)/ram
MULTICORE_DIR :=	$(RUNTIME_DIR)/hostthread
#
CLIB_DIR :=		$(RUNTIME_DIR)/lib
CONFIG_DIR :=		$(RUNTIME_DIR)/config

# CFLAGS =	-O -std=c99 -Wall -Werror
CFLAGS :=	-g3 -O2 -std=c99 -Wall -Werror
    #
    # -g version is useful for generating gdb-able coredumps.
    # Also need to comment out
    #     SET_SIGNAL_HANDLER( SIG_FAULT2, arithmetic_fault_handler );
    # in
    #     src/c/machine-dependent/posix-arithmetic-trap-handlers.c
    # NB: If we have an integer overflow during the run we'll coredump:
    #     we use synthesized SEGV signals to trap integer overflow.
    #
    # Drake Wilson advises:
    #     Note that -Wall is not a superset of -W.  -W has also been renamed 
    #     -Wextra for newer GCC versions.  -W -Wall is probably what you want 
    #     if your intention is "lots o' warnings". 
    #
    #     I also like to turn on -pedantic -fstrict-aliasing -fstrict-overflow 
    #     -Wstrict-aliasing=3 -Wstrict-overflow=4 -Wno-overlength-strings, but 
    #     you might or might not.  I also tend to use -Werror when developing, 
    #     but you might not like that either. 
    #
    # A webpage advises:
    #      -Werror -Wfatal-errors (that will stop after the first error, not trying to continue) 


INCLUDES :=	-I$(OBJS_DIR) -I$(H_DIR)
HEAPCLEANER_INCLUDES :=	$(INCLUDES) -I$(HEAPCLEANER_DIR)
GEN_INCLUDES :=	-I$(CONFIG_DIR) $(INCLUDES)

OBJS :=		mythryl-callable-c-libraries.o raise-error.o runtime-options.o \
		load-compiledfiles.o \
		load-and-run-heap-image.o run-mythryl-code-and-runtime-eventloop.o runtime-exception-stuff.o construct-runtime-package.o runtime-state.o \
		error-reporting.o ramlog.o timers.o posix-timers.o \
		swap-bytes.o \
		posix-arithmetic-trap-handlers.o signal-stuff.o interprocess-signals.o posix-profiling-support.o prim.o \
		$(XOBJS)


# Mythryl-visible interfaces to C libraries.
# Each of these has a string library name and
# exports some string-named functions which can
# be imported into the mythryl level via
#
#     my bar: This -> That
#           =  mythryl_callable_c_library_interface::find_c_function { lib_name => "foo", fun_name => "bar" };
#
# See:				src/lib/std/src/unsafe/mythryl-callable-c-library-interface.api
#				src/lib/std/src/unsafe/mythryl-callable-c-library-interface.pkg
#
# For a typical example see:	src/lib/std/src/psx/posix-file.pkg
#
CLIBS :=	$(XCLIBS) \
		$(CLIB_DIR)/posix-os/libmythryl-posix-os.a \
		$(CLIB_DIR)/heap/libmythryl-heap.a \
		$(CLIB_DIR)/kludge/libmythryl-kludge.a \
		$(CLIB_DIR)/signal/libmythryl-signal.a \
		$(CLIB_DIR)/space-and-time-profiling/libmythryl-space-and-time-profiling.a \
		$(CLIB_DIR)/socket/libmythryl-socket.a \
		$(CLIB_DIR)/time/libmythryl-time.a \
		$(CLIB_DIR)/date/libmythryl-date.a \
		$(CLIB_DIR)/math/libmythryl-math.a \
		$(CLIB_DIR)/hostthread/libmythryl-hostthread.a \
		$(CLIB_DIR)/posix-process/libmythryl-posix-process.a \
		$(CLIB_DIR)/posix-process-environment/libmythryl-posix-process-environment.a \
		$(CLIB_DIR)/posix-file-system/libmythryl-posix-file-system.a \
		$(CLIB_DIR)/posix-io/libmythryl-posix-io.a \
		$(CLIB_DIR)/posix-passwd/libmythryl-posix-passwd-db.a \
		$(CLIB_DIR)/posix-tty/libmythryl-posix-tty.a \
		$(CLIB_DIR)/posix-error/libmythryl-posix-error.a


# The mmap version is the default memory subsystem
#
LIBMEM_OBJS :=	'OBJS=$$(MMAP_OBJS)'


# The SGI version is the default multicore library	XXX BUGGO FIXME let's make this default to intel32 (when we have it!)
#
LIBMULTICORE_OBJS :=	'OBJS=$$(POSIX_THREADS_OBJS)'


# The default heapcleaner library:
#
HEAPCLEANER_LIB :=	libmythryl-heapcleaner.a


# The various libraries.
# Note that the order matters, since the C fns
# call heapcleaner library routines, and the
# heapcleaner library uses the memory library.
#
LIB7_LIBS :=	$(MULTICORE_DIR)/libmythryl-hostthread.a				\
		$(HEAPCLEANER_DIR)/$(HEAPCLEANER_LIB)					\
		$(RAM_DIR)/libmythryl-ram.a

STUFF_TO_REMOVE_WHEN_CLEANING :=							\
		generate-task-and-hostthread-struct-field-offsets-h			\
		generate-sizes-of-some-c-types-h					\
		interactive-mythryl-runtime-intel32					\
		task-and-hostthread-struct-field-offsets--autogenerated.h		\
		mythryl									\
		passthrough								\
		prim.s									\
		primops.s								\
		run.*									\
		sizes-of-some-c-types--autogenerated.h					\
		v-*

GENERATE_TASK_AND_HOSTTHREAD_STRUCT_FIELD_OFFSETS_H_DEPENDENCIES :=			\
		$(CONFIG_DIR)/generate-task-and-hostthread-struct-field-offsets-h.c	\
		start-and-finish-generating-header-file.o 				\
		$(CONFIG_DIR)/header-file-autogeneration-stuff.h			\
		mythryl passthrough set_heapdump_shebang				\
		sizes-of-some-c-types--autogenerated.h					\
		$(H_DIR)/runtime-base.h							\
		$(H_DIR)/system-dependent-root-register-indices.h

RUNTIME_LIBS := -pthread -lm

###################################################################################
# Do not edit this or following lines --- they are autobuilt.  (patchname="defs")

OPENGL_DIR :=		$(RUNTIME_DIR)/../opt/opengl
CLIBS      += $(OPENGL_DIR)/c/in-main/libmythryl-opengl.a

STUFF_TO_REMOVE_WHEN_CLEANING += mythryl-opengl-library-in-c-subprocess

GENERATE_TASK_AND_HOSTTHREAD_STRUCT_FIELD_OFFSETS_H_DEPENDENCIES += mythryl-opengl-library-in-c-subprocess



# Do not edit this or preceding lines --- they are autobuilt.
###################################################################################


DEP_LIBS :=	$(LIB7_LIBS) $(XLIBS)

ALL_LIBS :=	$(CLIBS) $(DEP_LIBS)





clean:
	@rm -f  *.o  $(STUFF_TO_REMOVE_WHEN_CLEANING)
	@(cd $(RAM_DIR);	    $(MAKE) MAKE="$(MAKE)" clean)
	@(cd $(CLIB_DIR);	    $(MAKE) MAKE="$(MAKE)" clean)
	@(cd $(HEAPCLEANER_DIR);    $(MAKE) MAKE="$(MAKE)" clean)
	@(cd $(MULTICORE_DIR);	    $(MAKE) MAKE="$(MAKE)" clean)


# 

$(RUNTIME):	$(VERSION) runtime-main.o $(OBJS) $(ALL_LIBS)
	echo "src/c/o/Makefile building $(RUNTIME):"
	$(CC) -o $(RUNTIME) $(CFLAGS) $(LDFLAGS) runtime-main.o $(OBJS) $(ALL_LIBS) $(LD_LIBS2) $(RUNTIME_LIBS) 

$(VERSION):
		echo "$(VERSION)" > $(VERSION)

#
# Sizes
#
sizes-of-some-c-types--autogenerated.h:	generate-sizes-of-some-c-types-h
	@echo "src/c/o/Makefile running 'generate-sizes-of-some-c-types-h' to generate sizes-of-some-c-types--autogenerated.h:"
	./generate-sizes-of-some-c-types-h

#
# main .compiled files
#
runtime-main.o:	$(MAIN_DIR)/runtime-main.c sizes-of-some-c-types--autogenerated.h $(H_DIR)/runtime-base.h $(H_DIR)/runtime-commandline-argument-processing.h $(H_DIR)/runtime-configuration.h $(H_DIR)/make-strings-and-vectors-etc.h
	$(CC) -c $(CFLAGS) $(DEFS) $(INCLUDES) $(MAIN_DIR)/runtime-main.c

runtime-options.o:	$(MAIN_DIR)/runtime-options.c $(H_DIR)/runtime-base.h $(H_DIR)/runtime-commandline-argument-processing.h sizes-of-some-c-types--autogenerated.h
	$(CC) -c $(CFLAGS) $(DEFS) $(INCLUDES) $(MAIN_DIR)/runtime-options.c

error-reporting.o:	$(MAIN_DIR)/error-reporting.c $(H_DIR)/runtime-base.h sizes-of-some-c-types--autogenerated.h
	$(CC) -c $(CFLAGS) $(DEFS) $(INCLUDES) $(MAIN_DIR)/error-reporting.c

ramlog.o:	$(MAIN_DIR)/ramlog.c $(H_DIR)/runtime-base.h sizes-of-some-c-types--autogenerated.h
	$(CC) -c $(CFLAGS) $(DEFS) $(INCLUDES) $(MAIN_DIR)/ramlog.c

dummy-main.o:	$(MAIN_DIR)/dummy-main.c sizes-of-some-c-types--autogenerated.h $(H_DIR)/runtime-base.h $(H_DIR)/runtime-configuration.h
	$(CC) -c $(CFLAGS) $(DEFS) $(INCLUDES) $(MAIN_DIR)/dummy-main.c

load-compiledfiles.o:		$(MAIN_DIR)/load-compiledfiles.c \
		$(H_DIR)/system-dependent-stuff.h $(H_DIR)/flush-instruction-cache-system-dependent.h \
		sizes-of-some-c-types--autogenerated.h $(H_DIR)/runtime-base.h $(H_DIR)/compiledfile.h \
		$(H_DIR)/make-strings-and-vectors-etc.h $(H_DIR)/runtime-globals.h $(H_DIR)/heapcleaner.h \
		$(H_DIR)/runtime-configuration.h
	$(CC) -c $(CFLAGS) $(DEFS) $(INCLUDES) $(MAIN_DIR)/load-compiledfiles.c

load-and-run-heap-image.o:	$(MAIN_DIR)/load-and-run-heap-image.c \
		sizes-of-some-c-types--autogenerated.h \
		$(H_DIR)/runtime-base.h $(H_DIR)/system-dependent-root-register-indices.h
	$(CC) -c $(CFLAGS) $(DEFS) $(INCLUDES) $(MAIN_DIR)/load-and-run-heap-image.c

run-mythryl-code-and-runtime-eventloop.o:	$(MAIN_DIR)/run-mythryl-code-and-runtime-eventloop.c \
		sizes-of-some-c-types--autogenerated.h \
		$(H_DIR)/runtime-base.h $(H_DIR)/runtime-values.h \
		$(H_DIR)/system-dependent-root-register-indices.h
	$(CC) -c $(CFLAGS) $(DEFS) $(INCLUDES) $(MAIN_DIR)/run-mythryl-code-and-runtime-eventloop.c

runtime-exception-stuff.o:	$(MAIN_DIR)/runtime-exception-stuff.c \
		sizes-of-some-c-types--autogenerated.h \
		$(H_DIR)/runtime-base.h $(H_DIR)/runtime-values.h \
		$(H_DIR)/system-dependent-root-register-indices.h
	$(CC) -c $(CFLAGS) $(DEFS) $(INCLUDES) $(MAIN_DIR)/runtime-exception-stuff.c

construct-runtime-package.o:	$(MAIN_DIR)/construct-runtime-package.c \
		sizes-of-some-c-types--autogenerated.h $(H_DIR)/runtime-base.h $(H_DIR)/runtime-values.h $(H_DIR)/runtime-configuration.h \
		$(H_DIR)/mythryl-callable-cfun-hashtable.h $(H_DIR)/architecture-and-os-names-system-dependent.h
	$(CC) -c $(CFLAGS) $(DEFS) $(INCLUDES) $(MAIN_DIR)/construct-runtime-package.c

runtime-state.o:	$(MAIN_DIR)/runtime-state.c \
		sizes-of-some-c-types--autogenerated.h \
		$(H_DIR)/runtime-base.h $(H_DIR)/heap-tags.h \
		$(H_DIR)/runtime-values.h $(H_DIR)/system-dependent-root-register-indices.h \
		$(H_DIR)/runtime-globals.h $(H_DIR)/runtime-timer.h $(H_DIR)/heapcleaner.h \
		$(H_DIR)/runtime-configuration.h
	$(CC) -c $(CFLAGS) $(DEFS) $(INCLUDES) $(MAIN_DIR)/runtime-state.c

timers.o:	$(MAIN_DIR)/timers.c \
		sizes-of-some-c-types--autogenerated.h \
		$(H_DIR)/runtime-base.h \
		$(H_DIR)/runtime-timer.h
	$(CC) -c $(CFLAGS) $(DEFS) $(INCLUDES) $(MAIN_DIR)/timers.c

posix-timers.o:	$(MAIN_DIR)/posix-timers.c \
		sizes-of-some-c-types--autogenerated.h \
		$(H_DIR)/runtime-base.h $(H_DIR)/system-dependent-unix-stuff.h \
		$(H_DIR)/runtime-timer.h
	$(CC) -c $(CFLAGS) $(DEFS) $(INCLUDES) $(MAIN_DIR)/posix-timers.c

swap-bytes.o:	$(MAIN_DIR)/swap-bytes.c \
		sizes-of-some-c-types--autogenerated.h $(H_DIR)/runtime-base.h
	$(CC) -c $(CFLAGS) $(DEFS) $(INCLUDES) $(MAIN_DIR)/swap-bytes.c


#
# C libraries
#
mythryl-callable-c-libraries.o:	$(CLIB_DIR)/mythryl-callable-c-libraries.c \
		sizes-of-some-c-types--autogenerated.h $(H_DIR)/runtime-base.h \
		$(CLIB_DIR)/mythryl-callable-c-libraries-list.h
	$(CC) -c $(CFLAGS) $(DEFS) $(INCLUDES) -I$(CLIB_DIR) $(CLIB_DIR)/mythryl-callable-c-libraries.c

raise-error.o:	$(CLIB_DIR)/raise-error.c \
		sizes-of-some-c-types--autogenerated.h \
		$(H_DIR)/runtime-base.h $(H_DIR)/system-dependent-root-register-indices.h \
		$(H_DIR)/make-strings-and-vectors-etc.h $(H_DIR)/runtime-globals.h \
		$(H_DIR)/system-dependent-unix-stuff.h
	$(CC) -c $(CFLAGS) $(DEFS) $(INCLUDES) $(CLIB_DIR)/raise-error.c

#
# Machine dependent .compiled files
#
posix-arithmetic-trap-handlers.o:	$(MACH_DIR)/posix-arithmetic-trap-handlers.c \
		sizes-of-some-c-types--autogenerated.h \
		$(H_DIR)/runtime-base.h $(H_DIR)/system-dependent-unix-stuff.h \
		$(H_DIR)/system-dependent-root-register-indices.h \
		$(H_DIR)/runtime-globals.h \
		$(H_DIR)/system-dependent-signal-get-set-etc.h
	$(CC) -c $(CFLAGS) $(DEFS) $(INCLUDES) $(MACH_DIR)/posix-arithmetic-trap-handlers.c

signal-stuff.o:	$(MACH_DIR)/signal-stuff.c \
		sizes-of-some-c-types--autogenerated.h \
		$(H_DIR)/runtime-base.h $(H_DIR)/system-dependent-signal-stuff.h \
		$(H_DIR)/system-dependent-root-register-indices.h \
		$(H_DIR)/runtime-configuration.h $(H_DIR)/make-strings-and-vectors-etc.h
	$(CC) -c $(CFLAGS) $(DEFS) $(INCLUDES) $(MACH_DIR)/signal-stuff.c

interprocess-signals.o:	$(MACH_DIR)/interprocess-signals.c \
		sizes-of-some-c-types--autogenerated.h \
		$(H_DIR)/runtime-base.h $(H_DIR)/system-dependent-unix-stuff.h \
		$(H_DIR)/system-dependent-signal-stuff.h $(H_DIR)/system-dependent-root-register-indices.h \
		$(H_DIR)/runtime-configuration.h $(H_DIR)/make-strings-and-vectors-etc.h \
		$(H_DIR)/runtime-globals.h \
		$(H_DIR)/system-dependent-signal-get-set-etc.h
	$(CC) -c $(CFLAGS) $(DEFS) $(INCLUDES) $(MACH_DIR)/interprocess-signals.c

posix-profiling-support.o:	$(MACH_DIR)/posix-profiling-support.c \
		sizes-of-some-c-types--autogenerated.h \
		$(H_DIR)/runtime-base.h $(H_DIR)/system-dependent-unix-stuff.h \
		$(H_DIR)/system-dependent-signal-stuff.h $(H_DIR)/runtime-globals.h \
		$(H_DIR)/system-dependent-signal-get-set-etc.h
	$(CC) -c $(CFLAGS) $(DEFS) $(INCLUDES) $(MACH_DIR)/posix-profiling-support.c

prim.o:		$(MACH_DIR)/prim.$(TARGET).asm \
		sizes-of-some-c-types--autogenerated.h $(H_DIR)/runtime-base.h $(H_DIR)/heap-tags.h \
		$(H_DIR)/asm-base.h \
		task-and-hostthread-struct-field-offsets--autogenerated.h
	$(CPP) -D_ASM_ $(DEFS) $(INCLUDES) $(MACH_DIR)/prim.$(TARGET).asm > prim.s
	$(AS) -o prim.o prim.s



#
#

task-and-hostthread-struct-field-offsets--autogenerated.h:	generate-task-and-hostthread-struct-field-offsets-h sizes-of-some-c-types--autogenerated.h $(H_DIR)/runtime-base.h $(H_DIR)/system-dependent-root-register-indices.h
	@echo "src/c/o/Makefile: Generating task-and-hostthread-struct-field-offsets--autogenerated.h by running generate-task-and-hostthread-struct-field-offsets-h:"
	./generate-task-and-hostthread-struct-field-offsets-h


# Arguments to recursive make:
#
MK_ARGS =		VERSION="$(VERSION)" \
			CC="$(CC)" CFLAGS="$(CFLAGS)" DEFS="$(DEFS)" \
			AR="$(AR)" ARFLAGS="$(ARFLAGS)" \
			RANLIB="$(RANLIB)" \
			INCLUDES="$(HEAPCLEANER_INCLUDES)"


# Library to allocate ram pages from host OS:
#
$(RAM_DIR)/libmythryl-ram.a: sizes-of-some-c-types--autogenerated.h
	(cd $(RAM_DIR); $(MAKE)  $(LIBMEM_OBJS) $(MK_ARGS) libmythryl-ram.a)


# Cleaner and heap I/O library:
#
$(HEAPCLEANER_DIR)/$(HEAPCLEANER_LIB): sizes-of-some-c-types--autogenerated.h
	(cd $(HEAPCLEANER_DIR); $(MAKE)  CHECK_HEAP="$(CHECK_HEAP)" $(MK_ARGS) $(HEAPCLEANER_LIB))



# Multicuore support library:
#
$(MULTICORE_DIR)/libmythryl-hostthread.a: sizes-of-some-c-types--autogenerated.h
	(cd $(MULTICORE_DIR); $(MAKE)  $(LIBMULTICORE_OBJS) $(MK_ARGS) libmythryl-hostthread.a)





# Wrappers to invoke compiler executable proper:
#


mythryl:	mythryl.c
	@echo "src/c/o/Makefile: Compiling mythryl ..."
	$(CC) $(CFLAGS) $(DEFS) -o mythryl mythryl.c
	@echo "src/c/o/Makefile: Installing ROOT/bin/mythryl:"
	cp mythryl ../../../bin
	chmod 755 ../../../bin/mythryl
	rm mythryl

#
# Miscellaneous tiny support programss:
#
passthrough:	passthrough.c
	@echo "src/c/o/Makefile: Compiling passthrough ..."
	$(CC) $(CFLAGS) $(DEFS) -o passthrough passthrough.c
	@echo "src/c/o/Makefile: Installing ROOT/bin/passthrough:"
	cp passthrough ../../../bin
	chmod 755 ../../../bin/passthrough
	rm passthrough


set_heapdump_shebang:	set-heapdump-shebang.c sizes-of-some-c-types--autogenerated.h $(ALL_LIBS)
	@echo "src/c/o/Makefile: Compiling set-heapdump-shebang ..."
	$(CC) $(CFLAGS) $(DEFS) $(ALL_LIBS) -I../h -I../heapcleaner -I../o -o set-heapdump-shebang set-heapdump-shebang.c
	@echo "src/c/o/Makefile: Installing ROOT/bin/set-heapdump-shebang:"
	cp set-heapdump-shebang ../../../bin
	chmod 755 ../../../bin/set-heapdump-shebang
	rm set-heapdump-shebang



#
# Configuration tools
#
generate-sizes-of-some-c-types-h:	$(CONFIG_DIR)/generate-sizes-of-some-c-types-h.c start-and-finish-generating-header-file.o $(CONFIG_DIR)/header-file-autogeneration-stuff.h $(H_DIR)/runtime-base.h
	$(CC) $(CFLAGS) $(DEFS) $(GEN_INCLUDES) -o generate-sizes-of-some-c-types-h $(CONFIG_DIR)/generate-sizes-of-some-c-types-h.c start-and-finish-generating-header-file.o

generate-regmask-h:	$(CONFIG_DIR)/generate-regmask-h.c start-and-finish-generating-header-file.o $(CONFIG_DIR)/header-file-autogeneration-stuff.h
	$(CC) $(CFLAGS) $(DEFS) $(GEN_INCLUDES) -o generate-regmask-h $(CONFIG_DIR)/generate-regmask-h.c start-and-finish-generating-header-file.o

# The "mythryl" "passthrough", "set_heapdump_shebang"  ... dependencies here are totally bogus,
# just a quick and (very!) dirty hack to get them compiled and installed:        XXX SUCKO FIXME

generate-task-and-hostthread-struct-field-offsets-h:	$(GENERATE_TASK_AND_HOSTTHREAD_STRUCT_FIELD_OFFSETS_H_DEPENDENCIES)
	$(CC) $(CFLAGS) $(DEFS) $(GEN_INCLUDES) -o generate-task-and-hostthread-struct-field-offsets-h $(CONFIG_DIR)/generate-task-and-hostthread-struct-field-offsets-h.c start-and-finish-generating-header-file.o

start-and-finish-generating-header-file.o:	$(CONFIG_DIR)/start-and-finish-generating-header-file.c $(CONFIG_DIR)/header-file-autogeneration-stuff.h
	$(CC) -c $(CFLAGS) $(GEN_INCLUDES) $(CONFIG_DIR)/start-and-finish-generating-header-file.c


#
# Make rules for the C libraries
#

# include directories for the library sub-directories
#
LIB_OBJS_DIR =	../../o
LIB_H_DIR =	../../h
LIB_INCLUDES =	-I$(LIB_OBJS_DIR) -I$(LIB_H_DIR) -I..

#
# Arguments to recursive make:
#
LIB_MK_ARGS =		VERSION="$(VERSION)" \
			CC="$(CC)" CFLAGS="$(CFLAGS)" DEFS="$(DEFS)" \
			AR="$(AR)" ARFLAGS="$(ARFLAGS)" \
			RANLIB="$(RANLIB)" \
			INCLUDES="$(LIB_INCLUDES)"

$(CLIB_DIR)/posix-os/libmythryl-posix-os.a: sizes-of-some-c-types--autogenerated.h
	(cd $(CLIB_DIR)/posix-os; $(MAKE)  $(LIB_MK_ARGS) libmythryl-posix-os.a)

$(CLIB_DIR)/heap/libmythryl-heap.a:  sizes-of-some-c-types--autogenerated.h
	(cd $(CLIB_DIR)/heap; $(MAKE)  $(LIB_MK_ARGS) libmythryl-heap.a)

$(CLIB_DIR)/kludge/libmythryl-kludge.a:  sizes-of-some-c-types--autogenerated.h
	(cd $(CLIB_DIR)/kludge; $(MAKE)  $(LIB_MK_ARGS) libmythryl-kludge.a)

$(CLIB_DIR)/signal/libmythryl-signal.a:  sizes-of-some-c-types--autogenerated.h
	(cd $(CLIB_DIR)/signal; $(MAKE)  $(LIB_MK_ARGS) libmythryl-signal.a)

$(CLIB_DIR)/space-and-time-profiling/libmythryl-space-and-time-profiling.a:  sizes-of-some-c-types--autogenerated.h
	(cd $(CLIB_DIR)/space-and-time-profiling; $(MAKE)  $(LIB_MK_ARGS) libmythryl-space-and-time-profiling.a)

$(CLIB_DIR)/socket/libmythryl-socket.a:  sizes-of-some-c-types--autogenerated.h
	(cd $(CLIB_DIR)/socket; $(MAKE)  $(LIB_MK_ARGS) libmythryl-socket.a)

$(CLIB_DIR)/time/libmythryl-time.a:  sizes-of-some-c-types--autogenerated.h
	(cd $(CLIB_DIR)/time; $(MAKE)  $(LIB_MK_ARGS) libmythryl-time.a)

$(CLIB_DIR)/date/libmythryl-date.a:  sizes-of-some-c-types--autogenerated.h
	(cd $(CLIB_DIR)/date; $(MAKE)  $(LIB_MK_ARGS) libmythryl-date.a)

$(CLIB_DIR)/math/libmythryl-math.a:  sizes-of-some-c-types--autogenerated.h
	(cd $(CLIB_DIR)/math; $(MAKE)  $(LIB_MK_ARGS) libmythryl-math.a)

$(CLIB_DIR)/hostthread/libmythryl-hostthread.a:  sizes-of-some-c-types--autogenerated.h
	(cd $(CLIB_DIR)/hostthread; $(MAKE)  $(LIB_MK_ARGS) libmythryl-hostthread.a)

$(CLIB_DIR)/posix-process/libmythryl-posix-process.a:  sizes-of-some-c-types--autogenerated.h
	(cd $(CLIB_DIR)/posix-process; $(MAKE)  $(LIB_MK_ARGS) libmythryl-posix-process.a)

$(CLIB_DIR)/posix-process-environment/libmythryl-posix-process-environment.a:  sizes-of-some-c-types--autogenerated.h
	(cd $(CLIB_DIR)/posix-process-environment; $(MAKE)  $(LIB_MK_ARGS) libmythryl-posix-process-environment.a)

$(CLIB_DIR)/posix-file-system/libmythryl-posix-file-system.a:  sizes-of-some-c-types--autogenerated.h
	(cd $(CLIB_DIR)/posix-file-system; $(MAKE)  $(LIB_MK_ARGS) libmythryl-posix-file-system.a)

$(CLIB_DIR)/posix-io/libmythryl-posix-io.a:  sizes-of-some-c-types--autogenerated.h
	(cd $(CLIB_DIR)/posix-io; $(MAKE)  $(LIB_MK_ARGS) libmythryl-posix-io.a)

$(CLIB_DIR)/posix-passwd/libmythryl-posix-passwd-db.a:  sizes-of-some-c-types--autogenerated.h
	(cd $(CLIB_DIR)/posix-passwd; $(MAKE)  $(LIB_MK_ARGS) libmythryl-posix-passwd-db.a)

$(CLIB_DIR)/posix-tty/libmythryl-posix-tty.a:  sizes-of-some-c-types--autogenerated.h
	(cd $(CLIB_DIR)/posix-tty; $(MAKE)  $(LIB_MK_ARGS) libmythryl-posix-tty.a)

$(CLIB_DIR)/posix-error/libmythryl-posix-error.a:  sizes-of-some-c-types--autogenerated.h
	(cd $(CLIB_DIR)/posix-error; $(MAKE)  $(LIB_MK_ARGS) libmythryl-posix-error.a)

$(CLIB_DIR)/dynamic-loading/libmythryl-dynamic-loading.a:  sizes-of-some-c-types--autogenerated.h
	(cd $(CLIB_DIR)/dynamic-loading; $(MAKE)  $(LIB_MK_ARGS) libmythryl-dynamic-loading.a)

$(CLIB_DIR)/ccalls/libmythryl-ccalls.a:  sizes-of-some-c-types--autogenerated.h
	(cd $(CLIB_DIR)/ccalls; $(MAKE)  $(LIB_MK_ARGS) libmythryl-ccalls.a)


###################################################################################
# Do not edit this or following lines --- they are autobuilt.  (patchname="rules")

mythryl-opengl-library-in-c-subprocess:	$(OPENGL_DIR)/c/in-sub/mythryl-opengl-library-in-c-subprocess.c
	@echo "src/c/o/Makefile: Compiling mythryl-opengl-library-in-c-subprocess ..."
	$(CC) $(CFLAGS) $(DEFS) -lglfw -lGLEW -lXrandr -lrt -lX11 -lGLU -lGL -pthread -lm  -o mythryl-opengl-library-in-c-subprocess  $(OPENGL_DIR)/c/in-sub/mythryl-opengl-library-in-c-subprocess.c
	@echo "src/c/o/Makefile: Installing ROOT/bin/mythryl-opengl-library-in-c-subprocess"
	cp mythryl-opengl-library-in-c-subprocess ../../../bin
	chmod 755 ../../../bin/mythryl-opengl-library-in-c-subprocess
	rm mythryl-opengl-library-in-c-subprocess

$(OPENGL_DIR)/c/in-main/libmythryl-opengl.a:  sizes-of-some-c-types--autogenerated.h
	(cd $(OPENGL_DIR)/c/in-main; $(MAKE)  $(LIB_MK_ARGS) libmythryl-opengl.a)

# Do not edit this or preceding lines --- they are autobuilt.
###################################################################################




## COPYRIGHT(c) 1994 AT&T Bell Laboratories.
## Subsequent changes by Jeff Prothero Copyright (c) 2010-2013,
## released per terms of SMLNJ-COPYRIGHT.
