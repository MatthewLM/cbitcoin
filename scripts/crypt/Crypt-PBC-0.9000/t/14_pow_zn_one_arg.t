# vi:fdm=marker fdl=0 syntax=perl:

use strict;
use Test;

if( defined $ENV{SKIP_ALL_BUT} ) { unless( $0 =~ m/\Q$ENV{SKIP_ALL_BUT}\E/ ) { plan tests => 1; skip(1); exit 0; } }

use Crypt::PBC;

my $curve = new Crypt::PBC('params_a.txt');

plan tests => 3;

my $x  = $curve->init_Zr->random;
my $P1 = $curve->init_G1->random;
my $P2 = clone $P1;

ok( $P1->is_eq( $P2 ) );
ok( $P1->clone->pow_zn( $x )->is_eq( $P2->pow_zn($P2, $x) ) );
ok( not $P1->is_eq( $P2 ) );
