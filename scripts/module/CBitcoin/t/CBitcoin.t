# Before `make install' is performed this script should be runnable with
# `make test'. After `make install' it should work as `perl CBitcoin.t'

#########################

# change 'tests => 1' to 'tests => last_test_to_print';

use strict;
use warnings;

use Test::More tests => 4;
BEGIN { use_ok('CBitcoin') };

#########################

# Insert your test code below, the Test::More module is use()ed here so read
# its man page ( perldoc Test::More ) for help writing this test script.

# 3 is the number of tests.
#use Test::More tests => 3;
#use CBitcoin;

is (CBitcoin::is_even(0), 1);
is (CBitcoin::is_even(1), 0);
is (CBitcoin::is_even(2), 1);
