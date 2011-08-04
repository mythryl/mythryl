#
#
# My normal development cycle consists of doing
# the following sequence of commands in the root
# directory (that containing the INSTALL file):
# 
#     make compiler		# Build the end-user compiler + libraries &tc.
#     make rest			# Build x-kit, c-kit, thread-kit ...
#     sudo make install		# Install new executables in /usr/bin so 'make check' will use them.
#     make check		# Verify basic sub/system operationality.
#     make tart			# Make a new full sourcecode tarball suitable for distribution
#				# in the directory above the root directory.


##-            "Coffee -- Black as hell, strong as death, sweet as love."
##-
##-                                               -- Turkish saying

nil:
	@echo "Do \"make help\" for help"

help:
	@echo
	@echo "This is the master makefile for Mythryl7, an advanced incrementally compiled language."
	@echo
	@echo
	@echo "The basic end-user oriented make commands are:"
	@echo
	@echo "    make mythryl7        # Build just the core executables and libraries."
	@echo "    make all             # Build the above plus various other goodies."
	@echo "    make clean           # Undo the above, returning filetree to pristine condition."
	@echo
	@echo
	@echo "One make command intended chiefly for compiler hackers is:"
	@echo
	@echo "    make id              # Build TAGS and ID files indexing the source files."
	@echo
	@echo "The current standard compiler hacker development cycle is:"
	@echo
	@echo "    make compiler        #"
	@echo "    make rest            #"
	@echo "    sudo make install    #"
	@echo "    make check           #"
	@echo "    make tart            # "
	@echo
	@echo "Other commands include:"
	@echo "    make install         # Install ./bin/ executables in   /usr/bin.  You probably need to be root."
	@echo "    make uninstall       # Remove  our    executables from /usr/bin.  You probably need to be root."
	@echo "    make comments 	# Update various cross-referencing comments in the codebase."

all:	c-stuff compiler rest

# We use   bin/mythryl-runtime-ia32   as a proxy for all
# of our C executables:
#
bin/mythryl-runtime-ia32: src/c/o/Makefile
	@sh/make-c-stuff
	@sh/patch-shebangs  bin/mythryld  bin/mythryl-lex  bin/mythryl-yacc

# User-friendly name for the above:
#
c-stuff: bin/mythryl-runtime-ia32

# This depends on a version of the gnu id-utils package
# hacked to understand SML syntax, available from
#     http://opensml.org/~cynbe/ml/smlnj-hacking-idutils.html
# NB: Currently the default map file gets overwritten regularly
# when updating system, so I have to do as root
#   cp ~cynbe/src/etc/id-utils/id-utils-3.2d/id-utils-3.2d-hacked/id-utils-3.2d/libidu/id-lang.map /usr/share/misc/id-lang.map
# to get it working again.

id_only:
	mkid-sml

etags:
	@-echo */*.pkg */*/*.pkg */*/*/*.pkg */*/*/*/*.pkg */*/*/*/*/*.pkg */*/*/*/*/*/*.pkg  */*/*/*/*/*/*/*.pkg \
              */*.api */*/*.api */*/*/*.api */*/*/*/*.api */*/*/*/*/*.api */*/*/*/*/*/*.api  */*/*/*/*/*/*/*.api \
              */*.unused */*/*.unused */*/*/*.unused */*/*/*/*.unused */*/*/*/*/*.unused */*/*/*/*/*/*.unused  */*/*/*/*/*/*/*.unused \
              */*.grammar */*/*.grammar */*/*/*.grammar */*/*/*/*.grammar */*/*/*/*/*.grammar */*/*/*/*/*/*.grammar  */*/*/*/*/*/*/*.grammar \
              */*.lex */*/*.lex */*/*/*.lex */*/*/*/*.lex */*/*/*/*/*.lex */*/*/*/*/*/*.lex  */*/*/*/*/*/*/*.lex \
              */*.tex */*/*.tex */*/*/*.tex */*/*/*/*.tex */*/*/*/*/*.tex */*/*/*/*/*/*.tex */*/*/*/*/*/*/*.tex \
              */*.mldoc */*/*.mldoc */*/*/*.mldoc */*/*/*/*.mldoc */*/*/*/*/*.mldoc */*/*/*/*/*/*.mldoc  */*/*/*/*/*/*/*.mldoc \
              */*.adl */*/*.adl */*/*/*.adl */*/*/*/*.adl */*/*/*/*/*.adl */*/*/*/*/*/*.adl */*/*/*/*/*/*/*.adl \
              */*.OVERVIEW */*/*.OVERVIEW */*/*/*.OVERVIEW */*/*/*/*.OVERVIEW */*/*/*/*/*.OVERVIEW */*/*/*/*/*/*.OVERVIEW  */*/*/*/*/*/*/*.OVERVIEW \
              */*.NOTES */*/*.NOTES */*/*/*.NOTES */*/*/*/*.NOTES */*/*/*/*/*.NOTES */*/*/*/*/*/*.NOTES  */*/*/*/*/*/*/*.NOTES \
              */*.README */*/*.README */*/*/*.README */*/*/*/*.README */*/*/*/*/*.README */*/*/*/*/*/*.README  */*/*/*/*/*/*/*.README  */*/*/*/*/*/*/*/*.README \
              */README* */*/README* */*/*/README* */*/*/*/README* */*/*/*/*/README* */*/*/*/*/*/README*  */*/*/*/*/*/*/README*  */*/*/*/*/*/*/*/README* \
              */build* */*/build* */*/*/build* */*/*/*/build* */*/*/*/*/build* */*/*/*/*/*/build*  */*/*/*/*/*/*/build*   */*/*/*/*/*/*/*/build* \
              */*.lib */*/*.lib */*/*/*.lib */*/*/*/*.lib */*/*/*/*/*.lib */*/*/*/*/*/*.lib  */*/*/*/*/*/*.lib   */*/*/*/*/*/*/*.lib \
              */*.sublib */*/*.sublib */*/*/*.sublib */*/*/*/*.sublib */*/*/*/*/*.sublib */*/*/*/*/*/*.sublib  */*/*/*/*/*/*.sublib   */*/*/*/*/*/*/*.sublib \
              Makefile */Makefile.in */*/Makefile.in */*/*/Makefile.in */*/*/*/Makefile.in */*/*/*/*/Makefile.in */*/*/*/*/*/Makefile.in  */*/*/*/*/*/*/Makefile.in   */*/*/*/*/*/*/*/Makefile.in \
              */makefile.win32 */*/makefile.win32 */*/*/makefile.win32 */*/*/*/makefile.win32 */*/*/*/*/makefile.win32 */*/*/*/*/*/makefile.win32 */*/*/*/*/*/*/makefile.win32 \
              */*.c */*/*.c */*/*/*.c */*/*/*/*.c */*/*/*/*/*.c */*/*/*/*/*/*.c  */*/*/*/*/*/*/*.c \
              */*.h */*/*.h */*/*/*.h */*/*/*/*.h */*/*/*/*/*.h */*/*/*/*/*/*.h  */*/*/*/*/*/*/*.h \
              */*.asm */*/*.asm */*/*/*.asm */*/*/*/*.asm */*/*/*/*/*.asm */*/*/*/*/*/*.asm  */*/*/*/*/*/*/*.asm \
             */*.masm */*/*.masm */*/*/*.masm */*/*/*/*.masm */*/*/*/*/*.masm */*/*/*/*/*/*.masm  */*/*/*/*/*/*/*.masm \
	     src/lib/compiler/back/low/pwrpc32/pwrpc32.architecture-description \
             src/lib/compiler/back/low/intel32/intel32.architecture-description \
             src/lib/compiler/back/low/sparc32/sparc32.architecture-description \
              sh/* \
              try/* \
              try/*/* \
	      src/lib/src/make-gtk-glue \
              src/lib/core/internal/version.template \
	      src/c/Configure.in \
             src/lib/html/html-gram \
             src/lib/core/init/init.cmi \
             src/lib/core/init/mythryl-primordial-library.cmi \
         | sed -e 's/ /\n/g' \
         | etags -               > /dev/null 2>&1

etags-makelib:
	echo src/lib/core/internal/version.template \
             src/lib/html/html-gram \
	     src/lib/html/htmythryl-lex \
              */*.lib */*/*.lib */*/*/*.lib */*/*/*/*.lib */*/*/*/*/*.lib */*/*/*/*/*/*.lib \
              */*.sublib */*/*.sublib */*/*/*.sublib */*/*/*/*.sublib */*/*/*/*/*.sublib */*/*/*/*/*/*.sublib \
              */*.makelib */*/*.makelib */*/*/*.makelib */*/*/*/*.makelib */*/*/*/*/*.makelib */*/*/*/*/*/*.makelib \
              */*.cm  */*/*.cm  */*/*/*.cm  */*/*/*/*.cm  */*/*/*/*/*.cm */*/*/*/*/*/*.cm \
         | sed -e 's/ /\n/g' \
         | etags -

id:	id_only etags

example:
	(cd src/example; ./make-example)

yacc-example:
	(cd src/yacc-example; ./make-yacc-example)

fixpoint:
	sh/make-fixpoint
	@echo "Starting next dev cycle   is usually next,  if \"Fixpoint reached in round 1\":"
	@echo "make tart;  make c-stuff;  make compiler;  make rest"
	@echo

update:
	(cd src/etc; ./installml)

tk:
	(cd src/lib/tk; make)

tarball: 
	sh/make-tarball

# Make a compressed tar archive containing
# the full source distribution.
#
tar:    clean tarball

# Same plus making tags files:
# 
tart:    clean tarball id			# "tart" == "tar + tags"

dist:   dist-clean

check:
	@MYTHRYL_ROOT=`pwd` sh/make-check

gtk-check:
	@MYTHRYL_ROOT=`pwd` sh/make-gtk-check

compiler-libraries: bin/mythryl-runtime-ia32
	@MYTHRYL_ROOT=`pwd` sh/make-compiler-libraries

compiler-libraries-soon:
	@MYTHRYL_ROOT=`pwd` sh/make-compiler-libraries-soon

compiler-executable:
	@MYTHRYL_ROOT=`pwd` sh/make-compiler-executable

compiler: compiler-libraries compiler-executable

compiler-soon: compiler-libraries-soon compiler-executable



# Re/build everything but the compiler
# and core compiler-critical libraries:
#
rest:	backends  save_yacc_and_lex  rest2  summary
r:	rest					# I'm a lazy typist.

# Three 'subroutines' for 'rest':
#
save_yacc_and_lex:
	mv bin/mythryl-yacc bin/mythryl-yacc.old
	mv bin/mythryl-lex  bin/mythryl-lex.old

rest2:
	$(MAKE) rest3

#	$(MAKE) --jobs=`grep processor /proc/cpuinfo | wc -l` rest3

# The list of apps and libs for
# 'make rest' to build.  This
# are non-core stuff that does not
# need to be in 'make compiler'.
#
# ORDER MATTERS!
#
# Don't re-order the following lines
# unless you know what you're doing.
#
rest3:	bin/mythryl-yacc \
	bin/mythryl-lex \
	bin/lexgen \
	bin/mythryl-burg-fraser-hanson-proebsting-92-optimal-tree-rewriter \
	bin/heap2asm \
	src/lib/posix/posix.lib.frozen \
	src/lib/reactive/reactive.lib.frozen \
	src/app/makelib/portable-graph/portable-graph-stuff.lib.frozen \
	src/lib/x-kit/xkit.lib.frozen \
	src/lib/compiler/back/low/lib/register-spilling.lib.frozen \
	src/lib/compiler/back/low/lib/peephole.lib.frozen \
	src/lib/compiler/back/low/lib/intel32-peephole.lib.frozen \
	src/lib/c-kit/src/c-kit.lib.frozen \
	src/lib/c-glue-lib/ram/memory.lib.frozen \
	src/lib/c-glue-lib/internals/c-internals.lib.frozen \
	src/lib/c-glue-lib/c.lib.frozen \
	src/lib/compiler/back/low/tools/line-number-database.lib.frozen \
	src/lib/compiler/back/low/tools/sml-ast.lib.frozen \
	src/lib/compiler/back/low/tools/precedence-parser.lib.frozen \
	src/lib/compiler/back/low/tools/architecture-parser.lib.frozen \
	src/lib/compiler/back/low/tools/match-compiler.lib.frozen \
	bin/c-glue-maker \
	bin/nowhere \
	src/lib/x-kit/tut/arithmetic-game/arithmetic-game-app.lib.frozen \
	src/lib/x-kit/tut/basicwin/basicwin-app.lib.frozen \
	src/lib/x-kit/tut/bitmap-editor/bitmap-editor.lib.frozen \
	src/lib/x-kit/tut/bouncing-heads/bouncing-heads-app.lib.frozen \
	src/lib/x-kit/tut/badbricks-game/badbricks-game-app.lib.frozen \
	src/lib/x-kit/tut/calculator/calculator-app.lib.frozen \
	src/lib/x-kit/tut/color-mixer/color-mixer-app.lib.frozen \
	src/lib/x-kit/tut/nbody/nbody-app.lib.frozen \
	src/lib/x-kit/tut/plaid/plaid-app.lib.frozen \
	src/lib/x-kit/tut/triangle/triangle-app.lib.frozen \
	src/lib/x-kit/tut/widget/widgets.lib.frozen \
	src/lib/x-kit/tut/show-graph/show-graph-app.lib.frozen


summary:
	@echo
	@echo "Compiled C programs:"
	@ls -l bin/mythryl-runtime-ia32 bin/mythryl bin/passthrough bin/set-heapdump-shebang bin/mythryl-gtk-slave
	@echo
	@echo "Main Mythryl compiler executable:"
	@ls -l bin/mythryld
	@echo
	@echo "Other compiled Mythryl programs:"
	@ls -l \
	       bin/c-glue-maker \
	       bin/heap2asm \
	       bin/lexgen \
	       bin/mythryl-burg-fraser-hanson-proebsting-92-optimal-tree-rewriter \
	       bin/mythryl-lex \
	       bin/mythryl-yacc \
	       bin/nowhere \

	@echo
	@echo "Non-core Mythryl freezefiles (compiled libraries):"
	@ls -l \
		src/app/makelib/portable-graph/portable-graph-stuff.lib.frozen \
		src/lib/c-glue-lib/c.lib.frozen \
		src/lib/c-glue-lib/internals/c-internals.lib.frozen \
		src/lib/c-glue-lib/ram/memory.lib.frozen \
		src/lib/c-kit/src/c-kit.lib.frozen \
		src/lib/compiler/back/low/lib/intel32-peephole.lib.frozen \
		src/lib/compiler/back/low/lib/peephole.lib.frozen \
		src/lib/compiler/back/low/lib/register-spilling.lib.frozen \
		src/lib/compiler/back/low/tools/match-compiler.lib.frozen \
		src/lib/compiler/back/low/tools/architecture-parser.lib.frozen \
		src/lib/compiler/back/low/tools/precedence-parser.lib.frozen \
		src/lib/compiler/back/low/tools/arch/make-sourcecode-for-backend-packages.lib.frozen \
		src/lib/compiler/back/low/tools/sml-ast.lib.frozen \
		src/lib/compiler/back/low/tools/line-number-database.lib.frozen \
		src/lib/reactive/reactive.lib.frozen \
		src/lib/posix/posix.lib.frozen \
		src/lib/x-kit/tut/arithmetic-game/arithmetic-game-app.lib.frozen \
		src/lib/x-kit/tut/basicwin/basicwin-app.lib.frozen \
		src/lib/x-kit/tut/bitmap-editor/bitmap-editor.lib.frozen \
		src/lib/x-kit/tut/bouncing-heads/bouncing-heads-app.lib.frozen \
		src/lib/x-kit/tut/badbricks-game/badbricks-game-app.lib.frozen \
		src/lib/x-kit/tut/calculator/calculator-app.lib.frozen \
		src/lib/x-kit/tut/color-mixer/color-mixer-app.lib.frozen \
		src/lib/x-kit/tut/nbody/nbody-app.lib.frozen \
		src/lib/x-kit/tut/plaid/plaid-app.lib.frozen \
		src/lib/x-kit/tut/triangle/triangle-app.lib.frozen \
		src/lib/x-kit/tut/widget/widgets.lib.frozen \
		src/lib/x-kit/tut/show-graph/show-graph-app.lib.frozen





# Stuff related to src/lib/src/make-gtk-glue:

gtk-glue:
	src/lib/src/make-gtk-glue

# The various individual apps and libraries
# which get built by 'make rest':

bin/mythryl-yacc:
	(cd src/app/yacc; ./build-yacc-app)

bin/mythryl-lex:
	(cd src/app/lex; ./build-lex-app)

bin/mythryl-burg-fraser-hanson-proebsting-92-optimal-tree-rewriter:	bin/mythryl-yacc bin/mythryl-lex
	(cd src/app/burg; ./build-mythryl-burg-fraser-hanson-proebsting-92-optimal-tree-rewriter-app)

bin/lexgen:
	(cd src/app/future-lex; ./build)

bin/heap2asm:
	(cd src/app/heap2asm; ./build-heap2asm-app)

bin/mythryl-gtk-slave:
	(cd src/c/o; make mythryl-gtk-slave)

src/lib/posix/posix.lib.frozen:
	@src/lib/posix/build-posix-lib

src/lib/reactive/reactive.lib.frozen:
	@src/lib/reactive/build-reactive-lib

src/app/makelib/portable-graph/portable-graph-stuff.lib.frozen:
	@src/app/makelib/portable-graph/build-portable-graph-stuff

src/lib/x-kit/xkit.lib.frozen:
	@src/lib/x-kit/build-xkit-lib

src/lib/compiler/back/low/lib/register-spilling.lib.frozen:
	@src/lib/compiler/back/low/lib/build-register-spilling-lib

src/lib/compiler/back/low/lib/peephole.lib.frozen:
	@src/lib/compiler/back/low/lib/build-peephole

src/lib/compiler/back/low/lib/intel32-peephole.lib.frozen:
	@src/lib/compiler/back/low/lib/build-ia32-peephole

src/lib/c-kit/src/c-kit.lib.frozen:
	@src/lib/c-kit/src/build

src/lib/c-glue-lib/ram/memory.lib.frozen:
	@src/lib/c-glue-lib/ram/build

src/lib/c-glue-lib/internals/c-internals.lib.frozen:   src/lib/c-glue-lib/ram/memory.lib.frozen
	@src/lib/c-glue-lib/internals/build

src/lib/c-glue-lib/c.lib.frozen:   src/lib/c-glue-lib/internals/c-internals.lib.frozen
	@src/lib/c-glue-lib/build



# This is a hack to make sure the backend code-synthesis logic
# gets exercised every build cycle even though the makelib::make
# logic is broken that would enable the
#     : shell (source: ../intel32/int32.architecture-description options:shared sh/make-sourcecode-for-backend-intel32)
# logic in
#     src/lib/compiler/back/low/intel32/backend-intel32.lib
# to function correctly:
#
backends:  # src/lib/compiler/back/low/intel32/backend-intel32.lib.frozen
	sh/make-sourcecode-for-backend-intel32
	sh/make-sourcecode-for-backend-pwrpc32
	sh/make-sourcecode-for-backend-sparc32


# Built.
src/lib/compiler/back/low/tools/line-number-database.lib.frozen:
	@src/lib/compiler/back/low/tools/build-source-map

# Apparently not run yet as part of 'make rest':
#
src/lib/compiler/back/low/tools/arch/make-sourcecode-for-backend-packages.lib.frozen:
	@src/lib/compiler/back/low/tools/build-architecture-generator

x: src/lib/compiler/back/low/tools/arch/make-sourcecode-for-backend-packages.lib.frozen

# Built.
src/lib/compiler/back/low/tools/sml-ast.lib.frozen:
	@src/lib/compiler/back/low/tools/build-sml-ast

# Built.
src/lib/compiler/back/low/tools/precedence-parser.lib.frozen:
	@src/lib/compiler/back/low/tools/build-precedence-parser

# Built.
src/lib/compiler/back/low/tools/architecture-parser.lib.frozen:
	@src/lib/compiler/back/low/tools/build-architecture-description-language-parser

# Built.
src/lib/compiler/back/low/tools/match-compiler.lib.frozen:
	@src/lib/compiler/back/low/tools/build-match-compiler

bin/c-glue-maker: src/lib/c-kit/src/c-kit.lib.frozen
	(cd src/app/c-glue-maker; ./build-c-glue-maker-app)

src/lib/x-kit/tut/arithmetic-game/arithmetic-game-app.lib.frozen:
	@src/lib/x-kit/tut/arithmetic-game/build-arithmetic-game-app

src/lib/x-kit/tut/basicwin/basicwin-app.lib.frozen:
	@src/lib/x-kit/tut/basicwin/build-basicwin-app

src/lib/x-kit/tut/bitmap-editor/bitmap-editor.lib.frozen:
	@src/lib/x-kit/tut/bitmap-editor/build-bitmap-editor

src/lib/x-kit/tut/bouncing-heads/bouncing-heads-app.lib.frozen:
	@src/lib/x-kit/tut/bouncing-heads/build-bouncing-heads-app

src/lib/x-kit/tut/badbricks-game/badbricks-game-app.lib.frozen:
	@src/lib/x-kit/tut/badbricks-game/build-badbricks-game-app

src/lib/x-kit/tut/calculator/calculator-app.lib.frozen:
	@src/lib/x-kit/tut/calculator/build-calculator-app

src/lib/x-kit/tut/color-mixer/color-mixer-app.lib.frozen:
	@src/lib/x-kit/tut/color-mixer/build-color-mixer-app

src/lib/x-kit/tut/nbody/nbody-app.lib.frozen:
	@src/lib/x-kit/tut/nbody/build-nbody-app

src/lib/x-kit/tut/plaid/plaid-app.lib.frozen:
	@src/lib/x-kit/tut/plaid/build-plaid-app

src/lib/x-kit/tut/triangle/triangle-app.lib.frozen:
	@src/lib/x-kit/tut/triangle/build-triangle-app

src/lib/x-kit/tut/widget/widgets.lib.frozen:
	@src/lib/x-kit/tut/widget/build-widgets

src/lib/x-kit/tut/show-graph/show-graph-app.lib.frozen:
	@src/lib/x-kit/tut/show-graph/build-show-graph-app

bin/nowhere: src/lib/compiler/back/low/tools/line-number-database.lib.frozen \
	     src/lib/compiler/back/low/tools/sml-ast.lib.frozen \
	     src/lib/compiler/back/low/tools/precedence-parser.lib.frozen \
	     src/lib/compiler/back/low/tools/architecture-parser.lib.frozen \
	     src/lib/compiler/back/low/tools/match-compiler.lib.frozen
	(cd src/lib/compiler/back/low/tools/nowhere; ./build-nowhere-app)



install:
	@sh/make-install

isntall:	install			# Man, I just can't type any more...

uninstall:
	@sh/uninstall


# Run the GNU autotools-generated Configure script
# to auto-configure the source code distribution
# to suit the current host.  This is almost
# always the first thing to do after unpacking
# the distribution sourcecode tarball.
#
# We use   src/c/o/Makefile   as a proxy for all
# the files generated for us by   src/c/Configure:
#
src/c/o/Makefile: src/c/Configure
	(cd src/c; ./Configure)
	src/c/check-for-gtk

# A more user-friendly name for the above:
#
configure: src/c/o/Makefile


# Run GNU autotools (autoconfig, mostly) to build
# site-customized makefiles, Configure file &tc.
#
# THIS IS RISKY unless you're the principal maintainer,
# or confident that you've got the same versions of the
# GNU autotools installed as the principal maintainer,
# since the GNU autotool people aren't big on upward
# compatibility or such.
#
# If you're not SURE you want to do this, you probably
# don't want to be doing it.
# 
# We use   src/c/Configure   as a proxy for all
# the files created for us by the gnu autotools:
# 
src/c/Configure:
	(cd src/c; ./make-gnu-autotools-output)

# A more user-friendly name for the above:
#
gnu-autotools-output: src/c/Configure

# The goat book ("Gnu autoconf, automake and libtool", Vaughan, Elliston, Tromey and Taylor)
# recommends 'bootstrap' as the name for the command which runs autoconfig & kith.
# I find that opaque, but let's support it as an alternate name, at least,
# for the benefit of anyone expecting it:
#
bootstrap:	gnu-autotools-output
gnu:		gnu-autotools-output

comments: MAKELIB_FILE_HIERARCHY.INFO~

MAKELIB_FILE_HIERARCHY.INFO~:
	  src/etc/mythryl-compiler-root.lib \
	  src/lib/core/mythryl-compiler-compiler/mythryl-compiler-compiler-for-this-platform.lib \
	  src/lib/x-kit/xkit.lib \
	  src/lib/c-kit/src/c-kit.lib \
	  src/lib/c-glue-lib/c.lib \
	  src/lib/c-glue-lib/internals/c-internals.lib \
	  src/lib/c-glue-lib/ram/memory.lib \
	  src/lib/compiler/back/low/lib/register-spilling.lib \
	  src/lib/compiler/back/low/lib/peephole.lib \
	  src/lib/compiler/back/low/lib/intel32-peephole.lib \
	  src/lib/posix/posix.lib \
	  src/lib/reactive/reactive.lib \
	  src/app/makelib/portable-graph/portable-graph-stuff.lib \
	  src/app/yacc/src/mythryl-yacc.lib \
	  src/app/lex/mythryl-lex.lib \
	  src/app/future-lex/src/lexgen.lib \
	  src/app/burg/mythryl-burg.lib \
	  src/app/heap2asm/heap2asm.lib \
	  src/app/c-glue-maker/c-glue-maker.lib \
	  src/lib/tk/src/sources.sublib \
	  src/lib/compiler/back/low/tools/line-number-database.lib \
	  src/lib/compiler/back/low/tools/arch/make-sourcecode-for-backend-packages.lib \
	  src/lib/compiler/back/low/tools/sml-ast.lib \
	  src/lib/compiler/back/low/tools/precedence-parser.lib \
	  src/lib/compiler/back/low/tools/architecture-parser.lib \
	  src/lib/compiler/back/low/tools/match-compiler.lib \
	  src/lib/compiler/back/low/tools/nowhere/nowhere.lib

# This is apparently not being compiled
# at present -- probably an oversight on my part:	XXX BUGGO FIXME
#	  src/lib/hash-consing/hash-cons.lib \


ppless:
	@-find . -type f -name '*.PRETTY_PRINT' -print | xargs rm -rf

somewhat-clean:	ppless
	@-(cd src/c/o;  make clean); # --no-print-directory 
	@-(cd src/lib/tk;  make clean) # --no-print-directory  
	@-(cd src/lib/c-glue;  make  clean) # --no-print-directory 
	@-(cd src/app/c-glue-maker;  make  clean)  # --no-print-directory 
	@-(cd src/lib/x-kit/tut/show-graph;  make  clean)  # --no-print-directory 
	@#-rm -rf bin
	@-rm -f core
	@-rm -f COMPILED_FILES_TO_LOAD
	@-rm -f LIBRARY_CONTENTS
	@-rm -rf glue;
	@-rm -rf .config config.sh UU;
	@find . -name '*~' -print | xargs rm -f;
	@find . -type f -name '*.frozen' -print | xargs rm -rf;
	@find . -type f -name '*.compiled' -print | xargs rm -rf;
	@find . -type f -name '*.module-dependencies-summary' -print | xargs rm -rf;
	@find . -type f -name '*[a-z].version' -print | xargs rm -rf;
	@find . -type f -name '*.index' -print | xargs rm -f;
	@find . -type f -name '*.load.log' -print | xargs rm -f;
	@find . -type f -name '*.compile.log' -print | xargs rm -f;
	@find . -type f -name '*.EDIT_REQUESTS' -print | xargs rm -f;
	@find . -type f -name '*.UNEDITED' -print | xargs rm -f;
	@find . -type f -name '*.EDITED' -print | xargs rm -f;
	@find . -type f -name '*.EDITS' -print | xargs rm -f;
	@find . -type f -name '*.SEEN' -print | xargs rm -f;
	@find . -type f -name '*.skeleton' -print | xargs rm -f;
	@find . -type f -name '*.log' -print | xargs rm -f;
#	@find . -type f -name '*.codemade.*' -print | xargs rm -f;
	@find . -type f -name 'tmp-makelib-pid-*' -print | xargs rm -f;
	@rm -rf sh/edit;
	@rm -rf src/etc/build7-compiledfiles;
	@rm -rf src/etc/build7.seed-libraries;
	@rm -rf src/etc/build7.boot.intel32-unix;
	@rm -rf src/etc/build7.intel32-linux;
	@rm -rf src/etc/build7-[1-9]-compiledfiles;
	@rm -rf src/etc/build7-[1-9].boot.intel32-unix;
	@rm -rf src/etc/build7-[1-9].seed-libraries;
	@rm -rf build7-compiledfiles;
	@rm -rf build7.seed-libraries;
	@rm -rf rm-rf-me;
	@rm -f mythryld;
	@rm -f src/lib/c-kit/src/parser/grammar/c.grammar.desc;
	@rm -f src/lib/c-kit/src/parser/grammar/c.grammar.api;
	@rm -f src/lib/c-kit/src/parser/grammar/c.grammar.pkg;
	@rm -f src/lib/c-kit/src/parser/grammar/c.lex.pkg;
	@rm -f src/lib/std/dot/dot-graph.grammar.desc
	@rm -f src/lib/std/dot/dot-graph.grammar.api
	@rm -f src/lib/std/dot/dot-graph.grammar.pkg
	@rm -f src/lib/std/dot/dot-graph.lex.pkg
	@rm -f src/app/makelib/parse/libfile.grammar.desc;
	@rm -f src/app/makelib/parse/libfile.grammar.api
	@rm -f src/app/makelib/parse/libfile.grammar.pkg;
	@rm -f src/app/makelib/parse/libfile.lex.pkg;
	@rm -f src/app/yacc/src/mythryl-yacc;
	@rm -f src/lib/compiler/front/parser/lex/mythryl.lex.pkg;
	@rm -f src/lib/compiler/front/parser/yacc/mythryl.grammar.desc;
	@rm -f src/lib/compiler/front/parser/yacc/mythryl.grammar.api;
	@rm -f src/lib/compiler/front/parser/yacc/mythryl.grammar.pkg;
	@rm -f src/lib/compiler/front/parser/lex/nada.lex.pkg;
	@rm -f src/lib/compiler/front/parser/yacc/nada.grammar.desc;
	@rm -f src/lib/compiler/front/parser/yacc/nada.grammar.api;
	@rm -f src/lib/compiler/front/parser/yacc/nada.grammar.pkg;
	@rm -f src/app/burg/burg.lex.pkg;
	@rm -f src/lib/compiler/back/low/tools/parser/adl.grammar.desc;
	@rm -f src/lib/compiler/back/low/tools/parser/adl.grammar.api;
	@rm -f src/lib/compiler/back/low/tools/parser/adl.grammar.pkg;
	@rm -f src/lib/compiler/back/low/tools/parser/adl.lex.pkg;
	@rm -f src/lib/html/html-gram.desc;
	@rm -f src/lib/html/html-gram.api;
	@rm -f src/lib/html/html-gram.pkg;
	@rm -f src/lib/html/html-lex.pkg;
	@rm -f src/app/future-lex/src/frontends/lex/mythryl-lex.grammar.api
	@rm -f src/app/future-lex/src/frontends/lex/mythryl-lex.grammar.pkg
	@rm -f src/c/cleaner/shebang-line.h;
	@rm -f src/c/o/mythryl-executable.h;
	@rm -f examples/c-tak;
	@rm -f examples/c-1000-strings;
	@rm -f src/lib/tk/src/sys_conf.pkg
	@# -rm src/lib/core/internal/version.pkg
	@# The following are too much of a good thing:
	@# -find . -name '*.grammar.desc' -print | xargs rm
	@# -find . -name '*.grammar.api' -print | xargs rm
	@# -find . -name '*.grammar.pkg' -print | xargs rm
	@# -find . -name '*.lex.pkg' -print | xargs rm

clean: somewhat-clean
	@-rm -f bin/*.old
	@-rm -f bin/nowhere
	@-rm -f bin/gtk-slave
	@-rm -f ID
	@-rm -f TAGS
	@-rm -f v-intel32-linux
	@-rm -f bin/mythryl-runtime-ia32
	@-rm -f bin/build-an-executable-mythryl-heap-image
	@-rm -f bin/c-glue-maker
	@-rm -f bin/guess-host-architecture-and-os
	@-rm -f bin/heap2asm
	@-rm -f bin/heap2exec
	@-rm -f bin/mythryl-ld
	@-rm -f bin/lexgen
	@-rm -f bin/makedepend7
	@-rm -f bin/mythryl-burg-fraser-hanson-proebsting-92-optimal-tree-rewriter
	@-rm -f bin/mythryl
	@-rm -f bin/passthrough
	@-rm -f bin/set-heapdump-shebang
	@-rm -f bin/mythryl-gtk-slave


# As above, but also remove the stuff generated by doing
#
#     make config
#
dist-clean:	clean
	@-find src -name 'Makefile' -print | xargs rm
	@-rm -f src/c/config.log
	@-rm -f src/c/config.status

# 'make rest' builds a lot of .frozen freezefiles (libraries), and consequently doing
# 'make rest' again does nothing even if the source files have been updated, because
# that is how freezefile semantics are specified.  This is a nuisance during development,
# so we define this make target to remove those .frozen files, allowing
# 'make rest' to recompile all changed (and dependent) sourcefiles:
#
rest-unfrozen:
	@-rm -f src/app/makelib/portable-graph/portable-graph-stuff.lib.frozen
	@-rm -f src/lib/c-glue-lib/c.lib.frozen
	@-rm -f src/lib/c-glue-lib/internals/c-internals.lib.frozen
	@-rm -f src/lib/c-glue-lib/ram/memory.lib.frozen
	@-rm -f src/lib/c-kit/src/c-kit.lib.frozen
	@-rm -f src/lib/compiler/back/low/lib/intel32-peephole.lib.frozen
	@-rm -f src/lib/compiler/back/low/lib/peephole.lib.frozen
	@-rm -f src/lib/compiler/back/low/lib/register-spilling.lib.frozen
	@-rm -f src/lib/compiler/back/low/tools/match-compiler.lib.frozen
	@-rm -f src/lib/compiler/back/low/tools/architecture-parser.lib.frozen
	@-rm -f src/lib/compiler/back/low/tools/precedence-parser.lib.frozen
	@-rm -f src/lib/compiler/back/low/tools/arch/make-sourcecode-for-backend-packages.lib.frozen
	@-rm -f src/lib/compiler/back/low/tools/sml-ast.lib.frozen
	@-rm -f src/lib/compiler/back/low/tools/line-number-database.lib.frozen
	@-rm -f src/lib/compiler/back/low/tools/line-number-database.lib.frozen
	@-rm -f src/lib/reactive/reactive.lib.frozen
	@-rm -f src/lib/posix/posix.lib.frozen
	@-rm -f src/lib/x-kit/tut/arithmetic-game/arithmetic-game-app.lib.frozen
	@-rm -f src/lib/x-kit/tut/basicwin/basicwin-app.lib.frozen
	@-rm -f src/lib/x-kit/tut/bitmap-editor/bitmap-editor.lib.frozen
	@-rm -f src/lib/x-kit/tut/bouncing-heads/bouncing-heads-app.lib.frozen
	@-rm -f src/lib/x-kit/tut/badbricks-game/badbricks-game-app.lib.frozen
	@-rm -f src/lib/x-kit/tut/calculator/calculator-app.lib.frozen
	@-rm -f src/lib/x-kit/tut/color-mixer/color-mixer-app.lib.frozen
	@-rm -f src/lib/x-kit/tut/show-graph/show-graph-app.lib.frozen
	@-rm -f src/lib/x-kit/tut/nbody/nbody-app.lib.frozen
	@-rm -f src/lib/x-kit/tut/plaid/plaid-app.lib.frozen
	@-rm -f src/lib/x-kit/tut/triangle/triangle-app.lib.frozen
	@-rm -f src/lib/x-kit/tut/widget/widgets.lib.frozen
	@-rm -f src/lib/x-kit/xkit.lib.frozen

# As above, but also remove the stuff generated by doing
#
#     make gnu-autotools-output
#
rm-gnu-autotools-output:	dist-clean
	@-rm -f src/c/Configure
	@-rm -f src/c/config.h

# Yeah, I'm lazy:
rmgnu: rm-gnu-autotools-output

check-glue-maker:
	(cd src/app/c-glue-maker;  make check)



# Need to do both 'make compiler'
# and 'make rest' before doing this:
#

nobook:


	@-echo "Document root URL is:"
	@-echo ""
	@-echo ""
	@-echo "You might want 'make html-tarball' or 'make bookpost' next"

# This is just a representative file
# produced by make-api-latex, the script
# which produces API defininitions directly
# from the internal compiler symbole tables:
#



# This is the original dump of the compiler
# symbol tables via make-api-reference, which
# in practice is largely obsoleted by
# make-api-latex.  Kept around for possible
# debugging or research use or whatever:
#
api-reference:

# Declaring phony targets as .PHONY ensures that they
# will function as expected even if someone creates a
# file by that name:
#
.PHONY: love bookpost nobook book testdoc check-glue-maker rmgnu \
        rm-gnu-autotools-output rest-unfrozen dist-clean clean   \
        somewhat-clean ppless comments gnu bootstrap             \
        gnu-autotools-output nil help all c-stuff id_only        \
        etags etags-makelib id example yacc-example fixpoint       \
        update tk tarball tar tart dist check gtk-check          \
        compiler-libraries compiler-libraries-soon               \
        compiler-executable compiler compiler-soon               \
        rest rest2 rest3 save_yacc_and_lex summary               \
        gtk-glue install isntall uninstall configure



#################################
# Attic: unused old code 

#
# This is my old version of id_only
# The codebase grew to the point where it hit
# the /bin/sh arguments-on-commandline limit,
# so I switched to the one 'id_only' above in the file:
id_only_explicit:
	mkid-sml */*.pkg */*/*.pkg */*/*/*.pkg */*/*/*/*.pkg */*/*/*/*/*.pkg \
                 */*.api */*/*.api */*/*/*.api */*/*/*/*.api */*/*/*/*/*.api \
                 */*.grammar */*/*.grammar */*/*/*.grammar */*/*/*/*.grammar */*/*/*/*/*.grammar \
                 */*.lex */*/*.lex */*/*/*.lex */*/*/*/*.lex */*/*/*/*/*.lex \
                 */*.lib */*/*.lib */*/*/*.lib */*/*/*/*.lib */*/*/*/*/*.lib


love:
	@echo "Not war?"
	@# In fond memory of the TOPS-10 'make' command. :)
# START: makedepend7 (src/app/debug/coverage.lib:poohbear); DO NOT DELETE!
poohbear: \
    src/app/debug/coverage.lib.frozen
# END  : makedepend7 (src/app/debug/coverage.lib:poohbear); DO NOT DELETE!
# START: makedepend7 (src/etc/mythryl-compiler-root.lib:eeyore); DO NOT DELETE!
eeyore: \
    src/etc/mythryl-compiler-root.lib \
    /pub/home/cynbe/src/mythryl/mythryl7/mythryl7.110.58/mythryl7.110.58/src/lib/tk/src/sources.sublib \
    /pub/home/cynbe/src/mythryl/mythryl7/mythryl7.110.58/mythryl7.110.58/src/lib/core/internal/interactive-system.lib.frozen \
    /pub/home/cynbe/src/mythryl/mythryl7/mythryl7.110.58/mythryl7.110.58/src/lib/core/init/init.cmi.frozen
# END  : makedepend7 (src/etc/mythryl-compiler-root.lib:eeyore); DO NOT DELETE!
# START: makedepend7 (src/lib/core/internal/srcpath.sublib:boojum); DO NOT DELETE!
boojum: \
    src/lib/core/internal/srcpath.sublib.frozen
# END  : makedepend7 (src/lib/core/internal/srcpath.sublib:boojum); DO NOT DELETE!
