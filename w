#! /usr/bin/perl -w
use strict;

# Remove bin/mythryld and rename bin/mythryld-bare to bin/mythryld-bootstrap:
#
if (-f "bin/mythryld") {
    system "rm -f bin/mythryld"                             and   die "Could not  rm -f bin/mythryld";
}

