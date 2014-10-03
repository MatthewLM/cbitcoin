 use strict;
 use warnings;
 use DBI;
 use Digest::SHA qw(sha256);
 use CBitcoin::CBHD;
 use CBitcoin::Script;
 use CBitcoin::Transaction;
 use CBitcoin::TransactionInput;
 use CBitcoin::TransactionOutput;
print "hello\n";

my $parentkey = new CBitcoin::CBHD;
$parentkey->serializedkeypair('xprv9s21ZrQH143K4Rz5APz2LhW4ms2mVTVa5YVMZzGAYgNRoXkxri6ELZVbzqc8VFtHseksaBnahJbbgkxue3nXsMjuF5pg1cknX4ueyQwUATY');
print "WIF:".$parentkey->WIF()."\n";
print "Address:".$parentkey->address()."\n";

my $script = CBitcoin::Script::address_to_script("1Fjf3xLuuQpuyw6EyxkXuLHzJUw7yUwPTi");
print "Script:$script\n";

my @in = ('KKWG2APFnnKg7S6uQdjtYUJ8eGMSHx6LA','PiMJNPBMzwQLucHp91aMVxpM8NA2o4Qm3');
foreach my $i1 (@in){
	print "Script:".CBitcoin::Script::address_to_script('1'.$i1)."\n";
}

__END__
#my $txinput = new CBitcoin::TransactionInput;
my $input = CBitcoin::TransactionInput::->new(
{
	'prevOutHash' => '06e595b5fe42b820f7c9762e8dd8fce26bcd83d7a48b184c0017bf49b6f0b5ad'
	,'prevOutIndex' => 1
	,'script' => CBitcoin::Script::address_to_script($parentkey->address())
}
);


my $btcamt = 3.4049993;
my $x1 = {
	 'value' => $btcamt*100000000
	,'script' => $script
};
my $output = CBitcoin::TransactionOutput::->new($x1);




my $tx = CBitcoin::Transaction::->new({});

$tx->addInput($input);
$tx->addOutput($output);

my $data = $tx->serializeData();
$data = $tx->serializeddata();
print "Unsigned Transaction Data:$data\n";

# sign an input
my $txdata = $tx->sign_single_input(0,$parentkey);
print "Final:$txdata\n";
#$data = $tx->serializeddata();
#print "Signed Transaction Data:$data\n";

my ($m, $n) = (2,3);
print "Let's do a multisig transaction of ($m, $n)\n";

my @arraypubkeys;
foreach my $i (1..$n){
	my $childkey = $parentkey->deriveChild(1,$i);
	print "Address $i:".$childkey->address()."\n";
	push(@arraypubkeys,$childkey->publickey());
}
print "Starting multisig operation\n";

$tx = CBitcoin::Transaction::->new();
$tx->addInput(CBitcoin::TransactionInput::->new(
{
	'prevOutHash' => '06e595b5fe42b820f7c9762e8dd8fce26bcd83d7a48b184c0017bf49b6f0b5ad'
	,'prevOutIndex' => 1
	,'script' => CBitcoin::Script::multisigToScript(\@arraypubkeys,$m,$n)
}
) );

# use the same output as before
$tx->addOutput($output);

$data = $tx->serializeData();
$data = $tx->serializeddata();
print "Unsigned Transaction Data:$data\n";
__END__
# sign with enough keys to validate the transaction
foreach my $i (0..($m-1)){
	my $childkey = $parentkey->deriveChild(1,$i+1);
	print "Ref:".ref($childkey)."\n";
	print "Address $i:".$childkey->address()."\n";
	$txdata = $tx->sign_single_input(0,$childkey);
	die "no tx data\n" unless $txdata;
	print "Latest:$txdata\n";
}

__END__

