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
#print "Script:$script\n";

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
my $oppppp = $tx->sign_single_input(0,$parentkey);
print "Final:$oppppp\n";
#$data = $tx->serializeddata();
#print "Signed Transaction Data:$data\n";

__END__
$x->{'data'} = CBitcoin::TransactionOutput::create_txoutput_obj(
	$x->{'script'}
	,$x->{'value'}
);
my $y = '';
$y = CBitcoin::TransactionOutput::get_value_from_obj($x->{'data'});
$y = $y/100000000;
print "Value:$y\n";
#$y = CBitcoin::TransactionInput::get_prevOutIndex_from_obj($x->{'data'});
#print "prevOutIndex:$y\n";
#$y = CBitcoin::TransactionInput::get_sequence_from_obj($x->{'data'});
#print "sequence:$y\n";
print "Data:".$x->{'data'}."\n";
$y = CBitcoin::TransactionOutput::get_script_from_obj($x->{'data'});
print "Script:$y\n";





