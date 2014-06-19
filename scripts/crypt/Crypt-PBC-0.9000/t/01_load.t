# vi:fdm=marker fdl=0 syntax=perl:

use strict;
use Test;

plan tests => 1;

eval {use Crypt::PBC; }; ok( not $@ );
