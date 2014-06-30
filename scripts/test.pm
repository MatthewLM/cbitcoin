 use strict;
 use warnings;
 use DBI;
 use Digest::SHA qw(sha256);
 use CBitcoin::Script;
 use CBitcoin::CBHD;
print "hello\n";
my ($m,$n) = (1,2);

my $script = CBitcoin::Script::address_to_script("1Fjf3xLuuQpuyw6EyxkXuLHzJUw7yUwPTi");
print "Script:$script\n";

my $parentkey = new CBitcoin::CBHD;
$parentkey->serializedkeypair('xprv9s21ZrQH143K4Rz5APz2LhW4ms2mVTVa5YVMZzGAYgNRoXkxri6ELZVbzqc8VFtHseksaBnahJbbgkxue3nXsMjuF5pg1cknX4ueyQwUATY');
$script = CBitcoin::Script::pubkeyToScript($parentkey->publickey());
my $scripttype = CBitcoin::Script::whatTypeOfScript($script);
print "Script from public key:$script\nType:$scripttype\n";
#my $childkey = $parentkey->deriveChild(1,23);

my @arraypubkeys;
foreach my $i (1..$n){
	my $childkey = $parentkey->deriveChild(1,$i);
	print "Address $i:".$childkey->address()."\n";
	push(@arraypubkeys,$childkey->publickey());
}
print "Starting multisig operation\n";
$script = CBitcoin::Script::multisigToScript(\@arraypubkeys,$m,$n);
print "Multisig Script:\n($script)\n";
$scripttype = CBitcoin::Script::whatTypeOfScript($script);

print "Script Type:$scripttype\n";


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
