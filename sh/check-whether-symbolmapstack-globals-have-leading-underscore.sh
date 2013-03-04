#!/bin/sh
#
#
# Check to see if "_" is prepended to global names in the symbol table.
#

CC=${CC:-cc}

TMP_FILE=/tmp/smlConfig-$$
TMP_FILE_C=$TMP_FILE.c

WITNESS="w3E_4Ew3E_4Rrr_56TtT"

cat > $TMP_FILE_C <<XXXX
$WITNESS () {}
XXXX

$CC -c -o $TMP_FILE $TMP_FILE_C
if [ "$?" != "0" ]; then
    rm -f $TMP_FILE $TMP_FILE_C
    exit 1
fi

if `nm $TMP_FILE | grep -q "_$WITNESS"`
  then echo "-DSYMBOLMAPSTACK_GLOBALS_HAVE_LEADING_UNDERSCORE"
fi             

rm -f $TMP_FILE $TMP_FILE_C

exit 0


# COPYRIGHT (c) 1995 AT&T Bell Laboratories.
# Subsequent changes by Jeff Prothero Copyright (c) 2010-2012,
# released per terms of SMLNJ-COPYRIGHT.

