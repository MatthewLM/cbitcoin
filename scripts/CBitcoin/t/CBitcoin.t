# Before `make install' is performed this script should be runnable with
# `make test'. After `make install' it should work as `perl CBitcoin.t'

#########################

# change 'tests => 1' to 'tests => last_test_to_print';

use strict;
use warnings;

use Test::More tests => 2;
BEGIN { use_ok('CBitcoin') };

#########################

# Insert your test code below, the Test::More module is use()ed here so read
# its man page ( perldoc Test::More ) for help writing this test script.

my $parentkey = new CBitcoin;
isa_ok( $parentkey, "CBitcoin" );

#my $bool = $parentkey->generate();
#ok( 1 + 1 == 2, "Key was successfully generated." );

1; 