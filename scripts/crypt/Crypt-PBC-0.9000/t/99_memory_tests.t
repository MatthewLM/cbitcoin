
use strict;
use Test;
use Crypt::PBC;

if( defined $ENV{SKIP_ALL_BUT} ) { unless( $0 =~ m/\Q$ENV{SKIP_ALL_BUT}\E/ ) { plan tests => 1; skip(1); exit 0; } }

eval "use Unix::Process";
if( $@ ) {
    plan tests => 1;
    skip( 1 );
    exit 0;
}

my $epochs = 50; # I tested this to 500 on my machine... 50 is prolly fine

plan tests => $epochs;

my $size = undef;
my $last = undef;
for (1 .. $epochs+4) { # we skip the first 4 (takes perl a while to calm down... *shrug*)
    SCOPE1: { 
        my $pair = new Crypt::PBC("params_d.txt");

        SCOPE2: {
            my $G1 = $pair->init_G1->random;
            my $G2 = $pair->init_G2->random;
            my $GT = $pair->init_GT->random;
            my $Zr = $pair->init_Zr->random;
        }
    }

    # This isn't the most accurate test in the whole wide world...
    # so don't be too shocked if it fails.
    $size = Unix::Process->vsz;

    ok( $size, $last ) if $_ > 4;

    $last = $size;
}
