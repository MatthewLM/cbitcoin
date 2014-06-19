# vi:fdm=marker fdl=0 syntax=perl:

use strict;
use Test;

if( defined $ENV{SKIP_ALL_BUT} ) { unless( $0 =~ m/\Q$ENV{SKIP_ALL_BUT}\E/ ) { plan tests => 1; skip(1); exit 0; } }

use Crypt::PBC;

my $curve = new Crypt::PBC("params_d.txt");

plan tests => 7;

my $a = $curve->init_Zr->set_to_int( 11 );
my $b = 11;
my $c = new Math::BigInt(11);

my $l1 = $curve->init_Zr->set_to_int( 5 )->mul(        $a );
my $l2 = $curve->init_Zr->set_to_int( 5 )->mul_zn(     $a );
my $l3 = $curve->init_Zr->set_to_int( 5 )->mul_int(    $b );
my $l4 = $curve->init_Zr->set_to_int( 5 )->mul_bigint( $c );
my $l5 = $curve->init_Zr->mul(        $curve->init_Zr->set_to_int(5), $a );
my $l6 = $curve->init_Zr->mul_zn(     $curve->init_Zr->set_to_int(5), $a );
my $l7 = $curve->init_Zr->mul_int(    $curve->init_Zr->set_to_int(5), $b );
my $l8 = $curve->init_Zr->mul_bigint( $curve->init_Zr->set_to_int(5), $c );

ok( $l1->is_eq( $l2 ) );
ok( $l1->is_eq( $l3 ) );
ok( $l1->is_eq( $l4 ) );
ok( $l1->is_eq( $l5 ) );
ok( $l1->is_eq( $l6 ) );
ok( $l1->is_eq( $l7 ) );
ok( $l1->is_eq( $l8 ) );
