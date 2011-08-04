##############################################################################
#
#  Template makefile for all ffi libraries 
#
#  Options for C compilation and linking
#
##############################################################################
CC=gcc
MAKE=make
CONFIGPATH=../config
C_GLUE_MAKER=c-glue-maker
SML=sml
SMLBINDIR=`which sml | sed -e 's|\(.*\)/[^/]*|\1|'`
GETARCH=$(SMLBINDIR)/.arch-n-opsys
VERSION=`grep 'val VERSION' make.pkg | sed -e 's/.*"\(.*\)".*/\1/'`
DEFAULT_CFLAGS= -U__GNUC__ -std=c99 -Wall 
MY_CFLAGS= $(CFLAGS) $(DEFAULT_CFLAGS)
MY_LDFLAGS= $(LDFLAGS)
SHARED_LIBS=$(SHARED_LIBRARIES:%=../lib/%)
LINKAGES=$(FFIS:%-ffi=../lib/%-lib.sml)
WRAPPER=make.pkg

#
#  Options for converting c header file to something digestable by 
#  c-glue-maker and Ckit
#
DEFINES=-D__extension__='' \
	-D__restrict='' \
	-D'__attribute__(x)'='' \
	-D'inline'='' \
	-D__builtin_va_list='void *'

#
#  Options for 
#

message:
	@echo "Version $(VERSION) of $(PROJECT)"
	@echo
	@echo "make runtime     -- generate the $(PROJECT) runtime system" 
	@echo "make ffi         -- generate the ffi to $(PROJECT)"
	@echo "make compile     -- compile the library"
	@echo "make stabilize   -- compile and stabilize the library"
	@echo "make restabilize -- restabilize the library"
	@echo "make wrapper     -- regenerate the 'wrapper' library"
	@echo "make include     -- regenerate $(H_FILE) from $(C_HEADERS)"
	@echo "make clean       -- clean out installation specific files"
	@echo "make cleanall    -- clean out all generated files"
	@echo "make all         -- same as make clean include runtime stabilize"
	@echo
	@echo "For simple installation: make all"
	@echo
	@echo "To create a new version of $(PROJECT) using $(C_HEADERS)"
	@echo "on your machine: make cleanall rebuild"
	@echo
	@echo "To do the same as above but don't bother stabilizing the"
	@echo "libraries:  make cleanall testrebuild"
	@if [ -f .stable ]; then echo "This library has been stablized"; fi

all:		check clean include runtime stabilize
rebuild:	check include runtime ffi wrapper stabilize
testrebuild:	check include runtime ffi wrapper compile
include:	rminclude $(H_FILE)

#
# sanity check
#
check:	
	@for exec in $(SML) $(C_GLUE_MAKER) $(CC) $(GETARCH) perl ; do \
	   prog=`which $$exec` ; \
	   if [ $$? != 0 ] ; then \
	      echo "The program '$$exec' is not found"; \
	      exit 1; \
	   fi; \
	done

rminclude:	
	/bin/rm -f $(H_FILE) $(H_FILE)-defines

libraries:	$(LIBRARIES)

runtime: $(SHARED_LIBS) $(LINKAGES)

../lib/%.so:	../lib
	eval "$(CC) -shared $(MY_LDFLAGS) -o $@"

../lib/%-lib.sml: ../lib
	$(CONFIGPATH)/make-lib $(@:../lib/%-lib.sml=libsml%.so) > $@

../lib:
	mkdir $@

ffi:	 $(FFIS)

#
#  How to make the ffi files from a .h file
#
%-ffi:  %.h 
	$(C_GLUE_MAKER) -light $(C_GLUE_MAKER_OPTS) -dir $@ -cmfile $@.cm \
	-include "../../lib/$(@:-ffi=-lib.sml) ../../common/smlnj-ffilib-basis.cm" $<

clean:	$(USERCLEAN)
	@/bin/rm -f .stable
	@/bin/rm -f $(SHARED_LIBS) $(LINKAGES)

cleanall:	clean	$(USERCLEANALL)
	@/bin/rm -f $(H_FILE) $(H_FILE)-defines
	@/bin/rm -rf $(FFIS)
	@/bin/rm -rf FFI 
	@/bin/rm -rf CM

$(H_FILE):	
	C_HEADERS='$(C_HEADERS)'; \
	echo $$C_HEADERS | \
	perl -p -e 's/(\S+)\s+/#include \1\n/g' | \
	$(CC) -E $(MY_CFLAGS) $(DEFINES) - |\
	perl -p -e $(H_FILTER) |\
	perl -p -e 's/^#.*"<.*>".*//' > $@

	if [ -f ignore-these-functions ] ; then \
	    mv $@ $@.tmp; \
            fgrep -v -f ignore-these-functions <$@.tmp > $@; \
	    /bin/rm $@.tmp; \
	fi

	echo '$(C_HEADERS:%=#include %@)' |\
	perl -p -e 's/@/\n/g' |\
	$(CC) -E -dDI $(MY_CFLAGS) - >$@-defines

wrapper: runtime ffi 
	echo 'use "$(WRAPPER)"' | $(SML)

../Undefines:
	mkdir $@	

compile:
	for f in $(CM_FILES) ; do \
	   echo Compiling "$$f"; \
	   echo 'os.Process.exit((if CM.make "'$$f'" then 0 else 1) handle _ => 1) : int' |\
	   $(SML); \
	   [ $$? == 0 ] || exit 1; \
	done

stabilize: 
	rm -f .stable
	for f in $(CM_FILES) ; do \
	   echo Compiling "$$f"; \
	   echo 'os.Process.exit((if CM.stabilize true "'$$f'" then 0 else 1) handle _ => 1) : int' |\
	   $(SML); \
	   [ $$? == 0 ] || exit 1; \
	done
	touch .stable

restabilize: clean-stable stabilize

clean-stable:
	rm -f .stable
	rm -f `find . -path '*/CM/*/*.cm'`

tar:
	DIR=`pwd`; \
	export DIR=`basename $$DIR`; \
	(cd ..; \
	find $$DIR -type f -print | egrep -v 'CM|CVS|Doc' | egrep -v '/$$' > $$DIR/MANIFEST; \
	tar -cf - -T $$DIR/MANIFEST) | \
	gzip >../sml-$$DIR-$(VERSION).tar.gz

version:
	@echo $(VERSION)

describe:
	@echo "$(PROJECT) [version $(VERSION)]"
	@echo
	@echo "rpm(s): $(RPMS)"
	@echo
	@if [ "$(RPMS)" != "" ] ; then rpm -qi $(RPMS); \
	elif [ -f README ]; then cat README; fi
	@echo 
	@echo CFLAGS='$(CFLAGS)'
	@echo LDFLAGS='$(LDFLAGS)'
	@echo '$(C_HEADERS)' | perl -p -e 's/(\S+)\s+/#include \1\n/g'

testconfig: 
	@$(MAKE) include 2>&1 >LOG || exit 1
	@echo '#include "$(H_FILE)"' > testconfig.c
	@echo 'int main() {return 0;}' >> testconfig.c
	@$(CC) -c testconfig.c >>LOG || exit 1
	@rm -f $(H_FILE) testconfig.c testconfig.o

dependson:
	@echo $(DEPENDS_ON)
