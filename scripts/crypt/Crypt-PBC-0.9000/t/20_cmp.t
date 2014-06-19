# vi:fdm=marker fdl=0 syntax=perl:

use strict;
use Test;

if( defined $ENV{SKIP_ALL_BUT} ) { unless( $0 =~ m/\Q$ENV{SKIP_ALL_BUT}\E/ ) { plan tests => 1; skip(1); exit 0; } }

use Crypt::PBC;

my $c   = new Crypt::PBC("params_d.txt");
my @all = ( $c->init_G1, $c->init_G2, $c->init_GT, $c->init_Zr );
my @noT = @all[0,1,3];

plan tests => 4*1 + @noT + 2;

for my $e ($all[3]) { # this used to test over 2 and 3, but when GT became Zr^k, 1=0 sorta...
    $e->set0; ok( $e->is0 and not $e->is1 );
    $e->set1; ok( $e->is1 and not $e->is0 );

    $e->set_to_int(0); ok( $e->is0 and not $e->is1 );
    $e->set_to_int(1); ok( $e->is1 and not $e->is0 );
}

for my $e (@noT) { # , $all[2]) {
    ok( $e->is_eq( $e ) );
}

ok( not $all[3]->set_to_int(19)->is_sqr );
ok(     $all[3]->set_to_int(25)->is_sqr );
