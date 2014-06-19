# vi:fdm=marker fdl=0 syntax=perl:

use strict;
use Test;

if( defined $ENV{SKIP_ALL_BUT} ) { unless( $0 =~ m/\Q$ENV{SKIP_ALL_BUT}\E/ ) { plan tests => 1; skip(1); exit 0; } }

use Crypt::PBC;

plan tests => 2;

my $symmetric  = new Crypt::PBC("params_a.txt");
my $asymmetric = new Crypt::PBC("params_d.txt");

my $G1_s = $symmetric->init_G1->random;
my $G2_s = $symmetric->init_G2->random;

my $G1_a = $asymmetric->init_G1->random;
my $G2_a = $asymmetric->init_G2->random;

eval { $G1_s->add( $G2_s ) }; ok( not $@ );
eval { $G1_a->add( $G2_a ) }; ok( $@ );
