 use strict;
 use warnings;
 use DBI;
 use Digest::SHA qw(sha256);
 use CBitcoin::Script;
 use CBitcoin::TransactionOutput;
print "hello\n";


my $script = CBitcoin::Script::address_to_script("1Fjf3xLuuQpuyw6EyxkXuLHzJUw7yUwPTi");
print "Script:$script\n";

#my $txinput = new CBitcoin::TransactionInput;
my $btcamt = 3.4049993;
my $x = {
	 'value' => $btcamt*100000000
	,'script' => $script
};
#my $txinput = CBitcoin::TransactionInput::->new($x);
my $txoutput = CBitcoin::TransactionOutput::->new($x);

print "Value:".($txoutput->value()/100000000)."\n";
print "Script:".$txoutput->script()."\n";
print "Data:".$txoutput->serializeddata()."\n";


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





