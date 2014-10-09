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
$parentkey->serialized_data('xprv9s21ZrQH143K4Rz5APz2LhW4ms2mVTVa5YVMZzGAYgNRoXkxri6ELZVbzqc8VFtHseksaBnahJbbgkxue3nXsMjuF5pg1cknX4ueyQwUATY');
print "WIF:".$parentkey->WIF()."\n";
print "Address:".$parentkey->address()."\n";

my $script = CBitcoin::Script::address_to_script("1Fjf3xLuuQpuyw6EyxkXuLHzJUw7yUwPTi");
print "Script:$script\n";

#my $txinput = new CBitcoin::TransactionInput;
my $input = CBitcoin::TransactionInput::->new(
{
	'prevOutHash' => '06e595b5fe42b820f7c9762e8dd8fce26bcd83d7a48b184c0017bf49b6f0b5ad'
	,'prevOutIndex' => '1'
	,'script' => CBitcoin::Script::address_to_script($parentkey->address())
}
);


my $btcamt = 3.4049993;
my $x1 = {
	 'value' => $btcamt*100000000
	,'script' => $script
};
my $output = CBitcoin::TransactionOutput::->new($x1);




my $tx = CBitcoin::Transaction::->new({'inputs' => [$input],'outputs' => [$output]});


my $data = $tx->serialized_data();
print "Unsigned Transaction Data:$data\n";

# sign an input
my $txdata = $tx->sign_single_input(0,$parentkey);
print "Final:$txdata\n";
$data = $tx->serialized_data();
print "Signed Transaction Data:$data\n";

my ($m, $n) = (2,3);
print "Let's do a multisig transaction of ($m, $n)\n";

my @arraypubkeys;
foreach my $i (1..$n){
	my $childkey = $parentkey->deriveChild(1,$i);
	print "Address $i:".$childkey->address()."\n";
	push(@arraypubkeys,$childkey);
}
print "Starting multisig operation\n";

print "Here is the script:".CBitcoin::Script::pubkeys_to_multisig_script(\@arraypubkeys,$m)."VERIFY\n";

$tx = CBitcoin::Transaction::->new({
	'inputs' => [
		CBitcoin::TransactionInput::->new({
			'prevOutHash' => '06e595b5fe42b820f7c9762e8dd8fce26bcd83d7a48b184c0017bf49b6f0b5ad'
			,'prevOutIndex' => 1
			,'script' => CBitcoin::Script::pubkeys_to_multisig_script(\@arraypubkeys,$m)
		})
		,CBitcoin::TransactionInput::->new({
                        'prevOutHash' => '06e595b5fe42b820f7c9762e8dd8fce26bcd83d7a48b184c0017bf49b6f0b5ad'
                        ,'prevOutIndex' => 1
                        ,'script' => CBitcoin::Script::pubkeys_to_multisig_script(\@arraypubkeys,$m)
                })
	]
	,'outputs' => [
		$output
	]
});

$data = $tx->serialized_data();
print "Unsigned Transaction Data:$data\n";

# sign with enough keys to validate the transaction
my $index = 0;
foreach my $i (@arraypubkeys){
	my $childkey = $i;#$parentkey->deriveChild(1,$i+1);
	print "Ref:".ref($childkey)."\n";
	print "Address $index:".$childkey->address()."\n";
	$txdata = $tx->sign_single_input($index,$childkey);
	die "no tx data\n" unless $txdata;
	print "Latest:$txdata\n";
	$index++;
}

__END__

