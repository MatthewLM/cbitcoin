# vi:fdm=marker fdl=0 syntax=perl:

use strict;
use Test;

if( defined $ENV{SKIP_ALL_BUT} ) { unless( $0 =~ m/\Q$ENV{SKIP_ALL_BUT}\E/ ) { plan tests => 1; skip(1); exit 0; } }

use Crypt::PBC;

my $curve = Crypt::PBC->new("params_d.txt");

my @lhs = ( $curve->init_G1, $curve->init_G2, $curve->init_GT, $curve->init_Zr );
my @rhs = ( $curve->init_G1, $curve->init_G2, $curve->init_GT, $curve->init_Zr );

my $epochs = 3;

plan tests => ( ((int @lhs) * 5 * $epochs) );

for my $i ( 1 .. $epochs ) {
    for my $i ( 0 .. $#lhs ) {

        $rhs[$i]->random;

        $lhs[$i]->set( $rhs[$i] )->square;           my $sc = $lhs[$i]->clone;
        $lhs[$i]->set( $rhs[$i] )->double;           my $dc = $lhs[$i]->clone;
        $lhs[$i]->set( $rhs[$i] )->halve;            my $hc = $lhs[$i]->clone;
        $lhs[$i]->set( $rhs[$i] ); $lhs[$i]->neg;    my $nc = $lhs[$i]->clone;
        $lhs[$i]->set( $rhs[$i] ); $lhs[$i]->invert; my $ic = $lhs[$i]->clone;

        $lhs[$i]->square( $rhs[$i] ); ok( $lhs[$i]->is_eq( $sc ) );
        $lhs[$i]->double( $rhs[$i] ); ok( $lhs[$i]->is_eq( $dc ) );
        $lhs[$i]->halve(  $rhs[$i] ); ok( $lhs[$i]->is_eq( $hc ) );
        $lhs[$i]->neg(    $rhs[$i] ); ok( $lhs[$i]->is_eq( $nc ) );
        $lhs[$i]->invert( $rhs[$i] ); ok( $lhs[$i]->is_eq( $ic ) );
    }
}
