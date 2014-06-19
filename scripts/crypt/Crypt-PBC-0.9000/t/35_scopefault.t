# vi:fdm=marker fdl=0 syntax=perl:

use strict;
use Test;

if( defined $ENV{SKIP_ALL_BUT} ) { unless( $0 =~ m/\Q$ENV{SKIP_ALL_BUT}\E/ ) { plan tests => 1; skip(1); exit 0; } }

plan tests => 1;

use Crypt::PBC;

###### ---- This test shouldn't sagfault.  If it does it's probably a bug in libpbc!

BIGGER_SCOPE: {
    my $e;

    HRM1: {
        my $c = new Crypt::PBC("params_d.txt");
           $e = $c->init_G1->set_to_hash( "lol!" );
    }

    HRM2: {
        my $d = new Crypt::PBC("params_d.txt");
        my $f = $d->init_G1->set_to_hash( "lol!" );
    }

    # warn " about to sagfault";
}
# warn " (sagfaulted so you don't see this)";

ok(1);
