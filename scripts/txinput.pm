 use strict;
 use warnings;
 use DBI;
 use Digest::SHA qw(sha256);
 use CBitcoin::Script;
 use CBitcoin::TransactionInput;
print "hello\n";


my $script = CBitcoin::Script::address_to_script("1Fjf3xLuuQpuyw6EyxkXuLHzJUw7yUwPTi");
print "Script:$script\n";

#my $txinput = new CBitcoin::TransactionInput;

my $x = {
	 'prevOutHash' => '06e595b5fe42b820f7c9762e8dd8fce26bcd83d7a48b184c0017bf49b6f0b5ad'
	,'prevOutIndex' => 10
	,'sequence' => 33 #hex('0xFFFFFFFF')
	,'script' => 'OP_DUP OP_HASH160 0xa1a2fea3e780c2d3f54acf41ea08ab580a2a620e OP_EQUALVERIFY OP_CHECKSIG'
};
#my $txinput = CBitcoin::TransactionInput::->new($x);
$x->{'data'} = CBitcoin::TransactionInput::create_txinput_obj(
	$x->{'script'}
	,$x->{'sequence'}
	,$x->{'prevOutHash'}
	,$x->{'prevOutIndex'}
);
my $y = '';
$y = CBitcoin::TransactionInput::get_prevOutHash_from_obj($x->{'data'});
print "PrevOutHash:$y\n";
$y = CBitcoin::TransactionInput::get_prevOutIndex_from_obj($x->{'data'});
print "prevOutIndex:$y\n";
$y = CBitcoin::TransactionInput::get_sequence_from_obj($x->{'data'});
print "sequence:$y\n";
print "Data:".$x->{'data'}."\n";
$y = CBitcoin::TransactionInput::get_script_from_obj($x->{'data'});
print "Script:$y\n";





