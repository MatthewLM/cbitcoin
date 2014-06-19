# vi:fdm=marker fdl=0 syntax=perl:

use strict;
use Test;

if( defined $ENV{SKIP_ALL_BUT} ) { unless( $0 =~ m/\Q$ENV{SKIP_ALL_BUT}\E/ ) { plan tests => 1; skip(1); exit 0; } }

use Crypt::PBC;

my $curve = new Crypt::PBC('params_d.txt');

plan tests => 50;

for ( 1 .. 50 ) {
    my $P_orig    = $curve->init_G2->random;
    my $P_bytes   = $P_orig->as_bytes;
    my $P_rebuilt = $curve->init_G2->set_to_bytes( $P_bytes );

    ok( $P_orig->is_eq( $P_rebuilt ) );
}
