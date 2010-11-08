#
#
# My normal development cycle consists of doing
# the following sequence of commands in the root
# directory (that containing the INSTALL file):
# 
#     make compiler            # Build the end-user compiler + libraries &tc.
#     make rest                # Build x-kit, c-kit, thread-kit ...
#     make tart                # Make a new full sourcecode tarball suitable for distribution
#                              # in the directory above the root directory.


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

# We use   bin/runtime7   as a proxy for all
# of our C executables:
#
bin/runtime7: src/runtime/o/Makefile
	@sh/make-c-stuff
	@sh/patch-shebangs  bin/mythryld  bin/mythryl-lex  bin/mythryl-yacc

# User-friendly name for the above:
#
c-stuff: bin/runtime7

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
              */*.grammar */*/*.grammar */*/*/*.grammar */*/*/*/*.grammar */*/*/*/*/*.grammar */*/*/*/*/*/*.grammar  */*/*/*/*/*/*/*.grammar \
              */*.lex */*/*.lex */*/*/*.lex */*/*/*/*.lex */*/*/*/*/*.lex */*/*/*/*/*/*.lex  */*/*/*/*/*/*/*.lex \
              */*.tex */*/*.tex */*/*/*.tex */*/*/*/*.tex */*/*/*/*/*.tex */*/*/*/*/*/*.tex */*/*/*/*/*/*/*.tex \
              */*.mldoc */*/*.mldoc */*/*/*.mldoc */*/*/*/*.mldoc */*/*/*/*/*.mldoc */*/*/*/*/*/*.mldoc  */*/*/*/*/*/*/*.mldoc \
              */*.OVERVIEW */*/*.OVERVIEW */*/*/*.OVERVIEW */*/*/*/*.OVERVIEW */*/*/*/*/*.OVERVIEW */*/*/*/*/*/*.OVERVIEW  */*/*/*/*/*/*/*.OVERVIEW \
              */README* */*/README* */*/*/README* */*/*/*/README* */*/*/*/*/README* */*/*/*/*/*/README*  */*/*/*/*/*/*/README* \
              */build* */*/build* */*/*/build* */*/*/*/build* */*/*/*/*/build* */*/*/*/*/*/build*  */*/*/*/*/*/*/build* \
              */*.make6 */*/*.make6 */*/*/*.make6 */*/*/*/*.make6 */*/*/*/*/*.make6 */*/*/*/*/*/*.make6  */*/*/*/*/*/*/*.make6 \
              */*.make7 */*/*.make7 */*/*/*.make7 */*/*/*/*.make7 */*/*/*/*/*.make7 */*/*/*/*/*/*.make7  */*/*/*/*/*/*.make7 \
              */*.cm  */*/*.cm  */*/*/*.cm  */*/*/*/*.cm  */*/*/*/*/*.cm */*/*/*/*/*/*.cm  */*/*/*/*/*/*/*.cm \
              Makefile */Makefile.in */*/Makefile.in */*/*/Makefile.in */*/*/*/Makefile.in */*/*/*/*/Makefile.in */*/*/*/*/*/Makefile.in  */*/*/*/*/*/*/Makefile.in \
              */makefile.win32 */*/makefile.win32 */*/*/makefile.win32 */*/*/*/makefile.win32 */*/*/*/*/makefile.win32 */*/*/*/*/*/makefile.win32 */*/*/*/*/*/*/makefile.win32 \
              */*.c */*/*.c */*/*/*.c */*/*/*/*.c */*/*/*/*/*.c */*/*/*/*/*/*.c  */*/*/*/*/*/*/*.c \
              */*.h */*/*.h */*/*/*.h */*/*/*/*.h */*/*/*/*/*.h */*/*/*/*/*/*.h  */*/*/*/*/*/*/*.h \
              */*.asm */*/*.asm */*/*/*.asm */*/*/*/*.asm */*/*/*/*/*.asm */*/*/*/*/*/*.asm  */*/*/*/*/*/*/*.asm \
             */*.masm */*/*.masm */*/*/*.masm */*/*/*/*.masm */*/*/*/*/*.masm */*/*/*/*/*/*.masm  */*/*/*/*/*/*/*.masm \
              sh/* \
              try/* \
              try/*/* \
              src/lib/core/internal/version.template \
	      src/runtime/Configure.in \
             src/lib/html/html-gram \
             src/lib/core/init/init.cmi \
         | sed -e 's/ /\n/g' \
         | etags -               > /dev/null 2>&1

etags-make7:
	echo src/lib/core/internal/version.template \
             src/lib/html/html-gram \
	     src/lib/html/htmythryl-lex \
              */*.make6 */*/*.make6 */*/*/*.make6 */*/*/*/*.make6 */*/*/*/*/*.make6 */*/*/*/*/*/*.make6 \
              */*.make7 */*/*.make7 */*/*/*.make7 */*/*/*/*.make7 */*/*/*/*/*.make7 */*/*/*/*/*/*.make7 \
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

compiler-libraries: bin/runtime7
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
rest:	save_yacc_and_lex rest2 summary
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
	bin/ml-burg \
	bin/heap2asm \
	src/lib/unix/unix-lib.make6.frozen \
	src/lib/reactive/reactive-lib.make6.frozen \
	src/app/make7/pgraph/pgraph-util.make6.frozen \
	src/lib/x-kit/x-kit.make6.frozen \
	src/lib/compiler/backend/lower/make7/ra.make6.frozen \
	src/lib/compiler/backend/lower/make7/peephole.make6.frozen \
	src/lib/compiler/backend/lower/make7/ia32-peephole.make6.frozen \
	src/lib/c-kit/src/c-kit-lib.make6.frozen \
	src/lib/c-glue-lib/memory/memory.make6.frozen \
	src/lib/c-glue-lib/internals/c-internals.make6.frozen \
	src/lib/c-glue-lib/c.make6.frozen \
	src/lib/compiler/backend/lower/tools/Lowcode-Prettyprinter.make6.frozen \
	src/lib/compiler/backend/lower/tools/source-map.make6.frozen \
	src/lib/compiler/backend/lower/tools/sml-ast.make6.frozen \
	src/lib/compiler/backend/lower/tools/prec-parser.make6.frozen \
	src/lib/compiler/backend/lower/tools/parser.make6.frozen \
	src/lib/compiler/backend/lower/tools/Match-Compiler.make6.frozen \
	bin/c-glue-maker \
	bin/nowhere \
	src/lib/x-kit/tut/arithmetic-game/arithmetic-game-app.make6.frozen \
	src/lib/x-kit/tut/basicwin/basicwin-app.make6.frozen \
	src/lib/x-kit/tut/bitmap-editor/bitmap-editor.make6.frozen \
	src/lib/x-kit/tut/bouncing-heads/bouncing-heads-app.make6.frozen \
	src/lib/x-kit/tut/badbricks-game/badbricks-game-app.make6.frozen \
	src/lib/x-kit/tut/calculator/calculator-app.make6.frozen \
	src/lib/x-kit/tut/color-mixer/color-mixer-app.make6.frozen \
	src/lib/x-kit/tut/nbody/nbody-app.make6.frozen \
	src/lib/x-kit/tut/plaid/plaid-app.make6.frozen \
	src/lib/x-kit/tut/triangle/triangle-app.make6.frozen \
	src/lib/x-kit/tut/widget/widgets.make6.frozen \
	src/lib/x-kit/tut/show-graph/show-graph-app.make6.frozen


summary:
	@echo
	@echo "Compiled C programs:"
	@ls -l bin/runtime7 bin/mythryl bin/passthrough bin/set-heapdump-shebang bin/mythryl-gtk-slave
	@echo
	@echo "Main Mythryl compiler executable:"
	@ls -l bin/mythryld
	@echo
	@echo "Other compiled Mythryl programs:"
	@ls -l \
	       bin/c-glue-maker \
	       bin/heap2asm \
	       bin/lexgen \
	       bin/ml-burg \
	       bin/mythryl-lex \
	       bin/mythryl-yacc \
	       bin/nowhere \

	@echo
	@echo "Non-core Mythryl freezefiles (compiled libraries):"
	@ls -l \
		src/app/make7/pgraph/pgraph-util.make6.frozen \
		src/lib/c-glue-lib/c.make6.frozen \
		src/lib/c-glue-lib/internals/c-internals.make6.frozen \
		src/lib/c-glue-lib/memory/memory.make6.frozen \
		src/lib/c-kit/src/c-kit-lib.make6.frozen \
		src/lib/compiler/backend/lower/make7/ia32-peephole.make6.frozen \
		src/lib/compiler/backend/lower/make7/peephole.make6.frozen \
		src/lib/compiler/backend/lower/make7/ra.make6.frozen \
		src/lib/compiler/backend/lower/tools/Lowcode-Prettyprinter.make6.frozen \
		src/lib/compiler/backend/lower/tools/Match-Compiler.make6.frozen \
		src/lib/compiler/backend/lower/tools/parser.make6.frozen \
		src/lib/compiler/backend/lower/tools/prec-parser.make6.frozen \
		src/lib/compiler/backend/lower/tools/sml-ast.make6.frozen \
		src/lib/compiler/backend/lower/tools/source-map.make6.frozen \
		src/lib/reactive/reactive-lib.make6.frozen \
		src/lib/unix/unix-lib.make6.frozen \
		src/lib/x-kit/tut/arithmetic-game/arithmetic-game-app.make6.frozen \
		src/lib/x-kit/tut/basicwin/basicwin-app.make6.frozen \
		src/lib/x-kit/tut/bitmap-editor/bitmap-editor.make6.frozen \
		src/lib/x-kit/tut/bouncing-heads/bouncing-heads-app.make6.frozen \
		src/lib/x-kit/tut/badbricks-game/badbricks-game-app.make6.frozen \
		src/lib/x-kit/tut/calculator/calculator-app.make6.frozen \
		src/lib/x-kit/tut/color-mixer/color-mixer-app.make6.frozen \
		src/lib/x-kit/tut/nbody/nbody-app.make6.frozen \
		src/lib/x-kit/tut/plaid/plaid-app.make6.frozen \
		src/lib/x-kit/tut/triangle/triangle-app.make6.frozen \
		src/lib/x-kit/tut/widget/widgets.make6.frozen \
		src/lib/x-kit/tut/show-graph/show-graph-app.make6.frozen

# Stuff related to src/lib/src/make-gtk-glue:

gtk-glue:
	src/lib/src/make-gtk-glue

# The various individual apps and libraries
# which get built by 'make rest':

bin/mythryl-yacc:
	(cd src/app/yacc; ./build)

bin/mythryl-lex:
	(cd src/app/lex; ./build)

bin/ml-burg:	bin/mythryl-yacc bin/mythryl-lex
	(cd src/app/burg; ./build)

bin/lexgen:
	(cd src/app/future-lex; ./build)

bin/heap2asm:
	(cd src/app/heap2asm; ./build)

bin/mythryl-gtk-slave:
	(cd src/runtime/o; make mythryl-gtk-slave)

src/lib/unix/unix-lib.make6.frozen:
	@src/lib/unix/build

src/lib/reactive/reactive-lib.make6.frozen:
	@src/lib/reactive/build

src/app/make7/pgraph/pgraph-util.make6.frozen:
	@src/app/make7/pgraph/build

src/lib/x-kit/x-kit.make6.frozen:
	@src/lib/x-kit/build

src/lib/compiler/backend/lower/make7/ra.make6.frozen:
	@src/lib/compiler/backend/lower/make7/build-ra

src/lib/compiler/backend/lower/make7/peephole.make6.frozen:
	@src/lib/compiler/backend/lower/make7/build-peephole

src/lib/compiler/backend/lower/make7/ia32-peephole.make6.frozen:
	@src/lib/compiler/backend/lower/make7/build-ia32-peephole

src/lib/c-kit/src/c-kit-lib.make6.frozen:
	@src/lib/c-kit/src/build

src/lib/c-glue-lib/memory/memory.make6.frozen:
	@src/lib/c-glue-lib/memory/build

src/lib/c-glue-lib/internals/c-internals.make6.frozen:   src/lib/c-glue-lib/memory/memory.make6.frozen
	@src/lib/c-glue-lib/internals/build

src/lib/c-glue-lib/c.make6.frozen:   src/lib/c-glue-lib/internals/c-internals.make6.frozen
	@src/lib/c-glue-lib/build

src/lib/compiler/backend/lower/tools/Lowcode-Prettyprinter.make6.frozen:
	@src/lib/compiler/backend/lower/tools/build-prettyprinter

src/lib/compiler/backend/lower/tools/source-map.make6.frozen:
	@src/lib/compiler/backend/lower/tools/build-source-map

src/lib/compiler/backend/lower/tools/sml-ast.make6.frozen:
	@src/lib/compiler/backend/lower/tools/build-sml-ast

src/lib/compiler/backend/lower/tools/prec-parser.make6.frozen:
	@src/lib/compiler/backend/lower/tools/build-prec-parser

src/lib/compiler/backend/lower/tools/parser.make6.frozen:
	@src/lib/compiler/backend/lower/tools/build-parser

src/lib/compiler/backend/lower/tools/Match-Compiler.make6.frozen:
	@src/lib/compiler/backend/lower/tools/build-match-compiler

bin/c-glue-maker: src/lib/c-kit/src/c-kit-lib.make6.frozen
	(cd src/app/c-glue-maker; ./build)

src/lib/x-kit/tut/arithmetic-game/arithmetic-game-app.make6.frozen:
	@src/lib/x-kit/tut/arithmetic-game/build-arithmetic-game-app

src/lib/x-kit/tut/basicwin/basicwin-app.make6.frozen:
	@src/lib/x-kit/tut/basicwin/build-basicwin-app

src/lib/x-kit/tut/bitmap-editor/bitmap-editor.make6.frozen:
	@src/lib/x-kit/tut/bitmap-editor/build-bitmap-editor

src/lib/x-kit/tut/bouncing-heads/bouncing-heads-app.make6.frozen:
	@src/lib/x-kit/tut/bouncing-heads/build-bouncing-heads-app

src/lib/x-kit/tut/badbricks-game/badbricks-game-app.make6.frozen:
	@src/lib/x-kit/tut/badbricks-game/build-badbricks-game-app

src/lib/x-kit/tut/calculator/calculator-app.make6.frozen:
	@src/lib/x-kit/tut/calculator/build-calculator-app

src/lib/x-kit/tut/color-mixer/color-mixer-app.make6.frozen:
	@src/lib/x-kit/tut/color-mixer/build-color-mixer-app

src/lib/x-kit/tut/nbody/nbody-app.make6.frozen:
	@src/lib/x-kit/tut/nbody/build-nbody-app

src/lib/x-kit/tut/plaid/plaid-app.make6.frozen:
	@src/lib/x-kit/tut/plaid/build-plaid-app

src/lib/x-kit/tut/triangle/triangle-app.make6.frozen:
	@src/lib/x-kit/tut/triangle/build-triangle-app

src/lib/x-kit/tut/widget/widgets.make6.frozen:
	@src/lib/x-kit/tut/widget/build-widgets

src/lib/x-kit/tut/show-graph/show-graph-app.make6.frozen:
	@src/lib/x-kit/tut/show-graph/build-show-graph-app

bin/nowhere: src/lib/compiler/backend/lower/tools/Lowcode-Prettyprinter.make6.frozen \
	     src/lib/compiler/backend/lower/tools/source-map.make6.frozen \
	     src/lib/compiler/backend/lower/tools/sml-ast.make6.frozen \
	     src/lib/compiler/backend/lower/tools/prec-parser.make6.frozen \
	     src/lib/compiler/backend/lower/tools/parser.make6.frozen \
	     src/lib/compiler/backend/lower/tools/Match-Compiler.make6.frozen
	(cd src/lib/compiler/backend/lower/tools/nowhere; ./build)



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
# We use   src/runtime/o/Makefile   as a proxy for all
# the files generated for us by   src/runtime/Configure:
#
src/runtime/o/Makefile: src/runtime/Configure
	(cd src/runtime; ./Configure)
	src/runtime/check-for-gtk

# A more user-friendly name for the above:
#
configure: src/runtime/o/Makefile


# Run GNU autotools (autoconfig, mostly) to build
# site-customized makefiles, Configure file &tc.
#
# THIS IS RISKY unless you're the principal maintainer,
# or confident that you've got the same versions of the
# GNU autotools installed as the principal maintainer,
# since the GNU autotool people aren't big on upward
# compatability or such.
#
# If you're not SURE you want to do this, you probably
# don't want to be doing it.
# 
# We use   src/runtime/Configure   as a proxy for all
# the files created for us by the gnu autotools:
# 
src/runtime/Configure:
	(cd src/runtime; ./make-gnu-autotools-output)

# A more user-friendly name for the above:
#
gnu-autotools-output: src/runtime/Configure

# The goat book ("Gnu autoconf, automake and libtool", Vaughan, Elliston, Tromey and Taylor)
# recommends 'bootstrap' as the name for the command which runs autoconfig & kith.
# I find that opaque, but let's support it as an alternate name, at least,
# for the benefit of anyone expecting it:
#
bootstrap:	gnu-autotools-output
gnu:		gnu-autotools-output

comments: MAKE7_FILE_HIERARCHY.INFO~

MAKE7_FILE_HIERARCHY.INFO~:
	  src/etc/root.make6 \
	  src/lib/core/make-compiler/current.make6 \
	  src/lib/x-kit/x-kit.make6 \
	  src/lib/c-kit/src/c-kit-lib.make6 \
	  src/lib/c-glue-lib/c.make6 \
	  src/lib/c-glue-lib/internals/c-internals.make6 \
	  src/lib/c-glue-lib/memory/memory.make6 \
	  src/lib/compiler/backend/lower/make7/ra.make6 \
	  src/lib/compiler/backend/lower/make7/peephole.make6 \
	  src/lib/compiler/backend/lower/make7/ia32-peephole.make6 \
	  src/lib/thread-kit/threadkit-lib/cm-descr/lib7.make6 \
	  src/lib/thread-kit/threadkit-lib/cm-descr/trace-threadkit.make6 \
	  src/lib/thread-kit/src/stdlib.make6 \
	  src/lib/thread-kit/src/threadkit.make6 \
	  src/lib/thread-kit/src/threadkit-internal.make6 \
	  src/lib/thread-kit/src/core-threadkit.make6 \
	  src/lib/unix/unix-lib.make6 \
	  src/lib/reactive/reactive-lib.make6 \
	  src/app/make7/pgraph/pgraph-util.make6 \
	  src/app/yacc/src/Mythryl-Yacc.make6 \
	  src/app/lex/mythryl-lex.make6 \
	  src/app/future-lex/src/Lexgen.make6 \
	  src/app/burg/Ml-Burg.make6 \
	  src/app/heap2asm/heap2asm.make6 \
	  src/app/c-glue-maker/c-glue-maker.make6 \
	  src/lib/tk/src/sources.make6 \
	  src/lib/compiler/backend/lower/tools/Lowcode-Prettyprinter.make6 \
	  src/lib/compiler/backend/lower/tools/source-map.make6 \
	  src/lib/compiler/backend/lower/tools/sml-ast.make6 \
	  src/lib/compiler/backend/lower/tools/prec-parser.make6 \
	  src/lib/compiler/backend/lower/tools/parser.make6 \
	  src/lib/compiler/backend/lower/tools/Match-Compiler.make6 \
	  src/lib/compiler/backend/lower/tools/nowhere/nowhere.make6

# This is apparently not being compiled
# at present -- probably an oversight on my part:	XXX BUGGO FIXME
#	  src/lib/hash-consing/hash-cons-lib.make6 \


ppless:
	@-find . -type f -name '*.PRETTY_PRINT' -print | xargs rm -rf

somewhat-clean:	ppless
	@-(cd src/runtime/o;  make clean); # --no-print-directory 
	@-(cd src/lib/tk;  make clean) # --no-print-directory  
	@-(cd src/lib/c-glue;  make  clean) # --no-print-directory 
	@-(cd src/app/c-glue-maker;  make  clean)  # --no-print-directory 
	@-(cd src/lib/x-kit/tut/show-graph;  make  clean)  # --no-print-directory 
	@#-rm -rf bin
	@-rm -f core
	@-rm -f OH7_FILES_TO_LOAD
	@-rm -f LIBRARY_CONTENTS
	@-rm -rf glue;
	@-rm -rf .config config.sh UU;
	@find . -name '*~' -print | xargs rm -f;
	@find . -type f -name '*.frozen' -print | xargs rm -rf;
	@find . -type f -name '*.o7' -print | xargs rm -rf;
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
	@find . -type f -name 'tmp-make7-pid-*' -print | xargs rm -f;
	@rm -rf sh/edit;
	@rm -rf src/etc/build7-o7-files;
	@rm -rf src/etc/build7.seed-libraries;
	@rm -rf src/etc/build7.boot.x86-unix;
	@rm -rf src/etc/build7.x86-linux;
	@rm -rf src/etc/build7-[1-9]-o7-files;
	@rm -rf src/etc/build7-[1-9].boot.x86-unix;
	@rm -rf src/etc/build7-[1-9].seed-libraries;
	@rm -rf build7-o7-files;
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
	@rm -f src/app/make7/parse/make7.grammar.desc;
	@rm -f src/app/make7/parse/make7.grammar.api
	@rm -f src/app/make7/parse/make7.grammar.pkg;
	@rm -f src/app/make7/parse/make7.lex.pkg;
	@rm -f src/app/yacc/src/mythryl-yacc;
	@rm -f src/lib/compiler/frontend/parse/lex/mythryl.lex.pkg;
	@rm -f src/lib/compiler/frontend/parse/yacc/mythryl.grammar.desc;
	@rm -f src/lib/compiler/frontend/parse/yacc/mythryl.grammar.api;
	@rm -f src/lib/compiler/frontend/parse/yacc/mythryl.grammar.pkg;
	@rm -f src/lib/compiler/frontend/parse/lex/nada.lex.pkg;
	@rm -f src/lib/compiler/frontend/parse/yacc/nada.grammar.desc;
	@rm -f src/lib/compiler/frontend/parse/yacc/nada.grammar.api;
	@rm -f src/lib/compiler/frontend/parse/yacc/nada.grammar.pkg;
	@rm -f src/app/burg/burg.lex.pkg;
	@rm -f src/lib/compiler/backend/lower/tools/parser/mdl.grammar.desc;
	@rm -f src/lib/compiler/backend/lower/tools/parser/mdl.grammar.api;
	@rm -f src/lib/compiler/backend/lower/tools/parser/mdl.grammar.pkg;
	@rm -f src/lib/compiler/backend/lower/tools/parser/mdl.lex.pkg;
	@rm -f src/lib/html/html-gram.desc;
	@rm -f src/lib/html/html-gram.api;
	@rm -f src/lib/html/html-gram.pkg;
	@rm -f src/lib/html/html-lex.pkg;
	@rm -f src/app/future-lex/src/front-ends/lex/mythryl-lex.grammar.api
	@rm -f src/app/future-lex/src/front-ends/lex/mythryl-lex.grammar.pkg
	@rm -f src/runtime/gc/shebang-line.h;
	@rm -f src/runtime/o/mythryl-executable.h;
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
	@-rm -f v-x86-linux
	@-rm -f bin/runtime7
	@-rm -f bin/build-an-executable-mythryl-heap-image
	@-rm -f bin/c-glue-maker
	@-rm -f bin/guess-host-architecture-and-os
	@-rm -f bin/heap2asm
	@-rm -f bin/heap2exec
	@-rm -f bin/ld7
	@-rm -f bin/lexgen
	@-rm -f bin/makedepend7
	@-rm -f bin/ml-burg
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
	@-rm -f src/runtime/config.log
	@-rm -f src/runtime/config.status

# 'make rest' builds a lot of .frozen freezefiles (libraries), and consequently doing
# 'make rest' again does nothing even if the source files have been updated, because
# that is how freezefile semantics are specified.  This is a nuisance during development,
# so we define this make target to remove those .frozen files, allowing
# 'make rest' to recompile all changed (and dependent) sourcefiles:
#
rest-unfrozen:
	@-rm -f src/app/make7/pgraph/pgraph-util.make6.frozen
	@-rm -f src/lib/c-glue-lib/c.make6.frozen
	@-rm -f src/lib/c-glue-lib/internals/c-internals.make6.frozen
	@-rm -f src/lib/c-glue-lib/memory/memory.make6.frozen
	@-rm -f src/lib/c-kit/src/c-kit-lib.make6.frozen
	@-rm -f src/lib/compiler/backend/lower/make7/ia32-peephole.make6.frozen
	@-rm -f src/lib/compiler/backend/lower/make7/peephole.make6.frozen
	@-rm -f src/lib/compiler/backend/lower/make7/ra.make6.frozen
	@-rm -f src/lib/compiler/backend/lower/tools/Lowcode-Prettyprinter.make6.frozen
	@-rm -f src/lib/compiler/backend/lower/tools/Match-Compiler.make6.frozen
	@-rm -f src/lib/compiler/backend/lower/tools/parser.make6.frozen
	@-rm -f src/lib/compiler/backend/lower/tools/prec-parser.make6.frozen
	@-rm -f src/lib/compiler/backend/lower/tools/sml-ast.make6.frozen
	@-rm -f src/lib/compiler/backend/lower/tools/source-map.make6.frozen
	@-rm -f src/lib/compiler/backend/lower/tools/source-map.make6.frozen
	@-rm -f src/lib/reactive/reactive-lib.make6.frozen
	@-rm -f src/lib/unix/unix-lib.make6.frozen
	@-rm -f src/lib/x-kit/tut/arithmetic-game/arithmetic-game-app.make6.frozen
	@-rm -f src/lib/x-kit/tut/basicwin/basicwin-app.make6.frozen
	@-rm -f src/lib/x-kit/tut/bitmap-editor/bitmap-editor.make6.frozen
	@-rm -f src/lib/x-kit/tut/bouncing-heads/bouncing-heads-app.make6.frozen
	@-rm -f src/lib/x-kit/tut/badbricks-game/badbricks-game-app.make6.frozen
	@-rm -f src/lib/x-kit/tut/calculator/calculator-app.make6.frozen
	@-rm -f src/lib/x-kit/tut/color-mixer/color-mixer-app.make6.frozen
	@-rm -f src/lib/x-kit/tut/show-graph/show-graph-app.make6.frozen
	@-rm -f src/lib/x-kit/tut/nbody/nbody-app.make6.frozen
	@-rm -f src/lib/x-kit/tut/plaid/plaid-app.make6.frozen
	@-rm -f src/lib/x-kit/tut/triangle/triangle-app.make6.frozen
	@-rm -f src/lib/x-kit/tut/widget/widgets.make6.frozen
	@-rm -f src/lib/x-kit/xkit.make6.frozen

# As above, but also remove the stuff generated by doing
#
#     make gnu-autotools-output
#
rm-gnu-autotools-output:	dist-clean
	@-rm -f src/runtime/Configure
	@-rm -f src/runtime/config.h

# Yeah, I'm lazy:
rmgnu: rm-gnu-autotools-output

check-glue-maker:
	(cd src/app/c-glue-maker;  make check)




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
        etags etags-make7 id example yacc-example fixpoint       \
        update tk tarball tar tart dist check gtk-check          \
        compiler-libraries compiler-libraries-soon               \
        compiler-executable compiler compiler-soon               \
        rest rest2 rest3 save_yacc_and_lex summary               \
        gtk-glue install isntall uninstall configure



#################################
# Attic: unused old code 

#
# This is my old version of id_only
# The codebase grew to the point where hit
# the /bin/sh arguments-on-commandline-limit,
# so I switched to the one 'id_only' above in the file:
id_only_explicit:
	mkid-sml */*.pkg */*/*.pkg */*/*/*.pkg */*/*/*/*.pkg */*/*/*/*/*.pkg \
                 */*.api */*/*.api */*/*/*.api */*/*/*/*.api */*/*/*/*/*.api \
                 */*.grammar */*/*.grammar */*/*/*.grammar */*/*/*/*.grammar */*/*/*/*/*.grammar \
                 */*.lex */*/*.lex */*/*/*.lex */*/*/*/*.lex */*/*/*/*/*.lex \
                 */*.make6 */*/*.make6 */*/*/*.make6 */*/*/*/*.make6 */*/*/*/*/*.make6


love:
	@echo "Not war?"
	@# In fond memory of the TOPS-10 'make' command. :)
# START: makedepend7 (src/app/debug/coverage.make6:poohbear); DO NOT DELETE!
poohbear: \
    src/app/debug/coverage.make6.frozen
# END  : makedepend7 (src/app/debug/coverage.make6:poohbear); DO NOT DELETE!
# START: makedepend7 (src/etc/root.make6:eeyore); DO NOT DELETE!
eeyore: \
    src/etc/root.make6 \
    /pub/home/cynbe/src/mythryl/mythryl7/mythryl7.110.58/mythryl7.110.58/src/lib/tk/src/sources.make6 \
    /pub/home/cynbe/src/mythryl/mythryl7/mythryl7.110.58/mythryl7.110.58/src/lib/core/internal/interactive-system.make6.frozen \
    /pub/home/cynbe/src/mythryl/mythryl7/mythryl7.110.58/mythryl7.110.58/src/lib/core/init/init.cmi.frozen
# END  : makedepend7 (src/etc/root.make6:eeyore); DO NOT DELETE!
# START: makedepend7 (src/lib/core/internal/srcpath-lib.make6:boojum); DO NOT DELETE!
boojum: \
    src/lib/core/internal/srcpath-lib.make6.frozen
# END  : makedepend7 (src/lib/core/internal/srcpath-lib.make6:boojum); DO NOT DELETE!
