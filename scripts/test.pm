 use strict;
 use warnings;
 use DBI;
 use Digest::SHA qw(sha256);
 use CBitcoin::CBHD;

print "hello\n";
my $parentkey = new CBitcoin::CBHD;
$parentkey->generate();
my ($wif,$address) = ($parentkey->WIF(),$parentkey->address());
print "WIF:$wif\n";
print "Address:$address\n";

my $childkey = $parentkey->deriveChild(1,2);
($wif,$address) = ($childkey->WIF(),$childkey->address());

print "WIF:$wif\n";
print "Address:$address\n";

