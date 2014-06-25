 use CBitcoin::Script;

my $address = '1ErayvrvUhxw1Nr6gkWmqVHUNZdX5QxxjY';

print "Script:".CBitcoin::Script::addressToScript($address)."\n";
