 use strict;
 use warnings;
 use DBI;
 use Digest::SHA qw(sha256);
 use CBitcoin::Script;
 use CBitcoin::CBHD;
print "hello\n";


my $script = CBitcoin::Script::address_to_script("1Fjf3xLuuQpuyw6EyxkXuLHzJUw7yUwPTi");
print "Script:$script\n";

my $parentkey = new CBitcoin::CBHD;
$parentkey->generate();
$script = CBitcoin::Script::pubkeyToScript($parentkey->publickey());
print "Script from public key:$script\n";
#my $childkey = $parentkey->deriveChild(1,23);

my @arraypubkeys;
foreach my $i (1..3){
	my $childkey = $parentkey->deriveChild(1,$i);
	print "Address $i:".$childkey->address()."\n";
	push(@arraypubkeys,$childkey->publickey());
}
print "Starting multisig operation\n";
$script = CBitcoin::Script::multisigToScript(\@arraypubkeys,2,3);
print "Multisig Script:$script\n";
__END__

=pod
my $parentkey = new CBitcoin::CBHD;
$parentkey->generate();
my ($wif,$address) = ($parentkey->WIF(),$parentkey->address());
print "WIF:$wif\n";
print "Address:$address\n";

my $childkey = $parentkey->deriveChild(1,2);
($wif,$address) = ($childkey->WIF(),$childkey->address());

print "WIF:$wif\n";
print "Address:$address\n";
=cut
