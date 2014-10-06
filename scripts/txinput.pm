 use strict;
 use warnings;
 use DBI;
 use Digest::SHA qw(sha256);
 use CBitcoin::CBHD;
 use CBitcoin::Script;
 use CBitcoin::Transaction;
 use CBitcoin::TransactionInput;
 use CBitcoin::TransactionOutput;
 use Data::Dumper;
print "hello\n";

my @last;
my $parentkey = CBitcoin::CBHD->new();
#$parentkey->generate();
print "Seed:".$parentkey->serialized_data('xprv9s21ZrQH143K31cxYHUBPB5qkjbhJ7X4HpqeM5czartEsaCZ9Vmp46tpq9XnVhHTzmYgLhHF3uDmXaLq9Qsz82PB3Rp3fAqrFHAWXWu9mvE')."\n";
my $i = 1;
for (<DATA>) {
	next if /^#/;
	chomp;
	unless(scalar(@last) > 0){
		@last = split(',',$_);
		next;
	}
	eval {
		#use bigint;
		my $key = $parentkey->deriveChild(1,$i);
		my $fromkey = $parentkey->deriveChild(1,1099924-$i);

		my ($address,$value,$prevOutHash,$prevOutIndex) = split(',',$_);
		$address = $fromkey->address();
		$i++;
		my ($lastaddress,$lastvalue) = ($key->address,$value-13200);
		my $script = CBitcoin::Script::address_to_script($address);
		#my $newaddress = CBitcoin::Script::script_to_address($script);

		my $in = CBitcoin::TransactionInput->new({
			'prevOutHash' => $prevOutHash
			,'prevOutIndex' => $prevOutIndex
			,'script' => $script
		});
		my $in2 = CBitcoin::TransactionInput->new({
			'prevOutHash' => 'b6b3954b661bf287789c19825682ebeba1ed08f5864b2a3902bc7aff4b02f775'
			,'prevOutIndex' => $prevOutIndex+3
			,'script' => $script
		});
		$in->script;$in->prevOutHash;$in->prevOutIndex;$in->prevOutHash;
		#warn "In:".$in->serialized_data."(".length($in->serialized_data).")\n";
		my $lastscript = CBitcoin::Script::address_to_script($lastaddress);
		my $out = CBitcoin::TransactionOutput->new({'script' => $lastscript, 'value' => $lastvalue});			
		my $bool = 0;

		#warn "Out:".$out->serialized_data."(".length($out->serialized_data).")\n";
		my $tx = CBitcoin::Transaction->new({
			'inputs' => [$in,$in2]
			,'outputs' => [$out]
		});

		my $predata = $tx->serialized_data;
		$tx->sign_single_input(0,$fromkey);
		$tx->sign_single_input(1,$fromkey);
		my $postdata = $tx->serialized_data;
		die "Data hasn't changed\n" if $predata eq $postdata;

		if(
			$in->prevOutHash eq $tx->input(0)->prevOutHash
			&& $out->script eq $tx->output(0)->script
			#&& $in->prevOutHash eq $prevOutHash
			&& CBitcoin::Script::script_to_address($out->script) eq $lastaddress
		){
			$bool = 1;
		}

		my $po = {
			'data' => $tx->serialized_data
			,'tx->in prevOutHash' => $tx->input(0)->prevOutHash()
			,'in prevOutHash' => $in->prevOutHash()
			,'prevOutHash' => $prevOutHash
			,'prevOutIndex' => $prevOutIndex
			,'destination' => [$lastaddress,$lastvalue]
		};
		my $xo = Data::Dumper::Dumper($po);
		print "Test:$bool\n$xo\n\n";
		@last = ($address,$value,$prevOutHash,$prevOutIndex);
	};
	if($@){ warn "Error:$@";}
	last;
}



__DATA__
# The following keys were created using javascript code on bitaddress.org
16PRgUZvCneM7AYJ94TaoGE4rMQnRdqqt4,5453543543,3f12ca91d24d3ff8eb89058c3ab05433419ea069ed31b754eaf4afa7486f4f8f,0
1KDxAxej4NZMQtao9xZGiadbsqxcKJt9Ng,1000933,3f12ca91d24d3ff8eb89058c3ab05433419ea069ed31b754eaf4afa7486f4f8f,0
1PnA88ck7hGSsSqpPXhaVbWL3suWXEqfsF,100033223,3f12ca91d24d3ff8eb89058c3ab05433419ea069ed31b754eaf4afa7486f4f8f,2
12jXM28Awqgm2NPgiD6EVjZmih66U5mUAt,99599595,3f12ca91d24d3ff8eb89058c3ab05433419ea069ed31b754eaf4afa7486f4f8f,5
1BV1GXQmBKF6v6CqUjH6y95KNXqtCoLNWH,345445,3f12ca91d24d3ff8eb89058c3ab05433419ea069ed31b754eaf4afa7486f4f8f,0
1AsCoc5UVkHcfFRXUA2YtDJbse7k6Ws4Pz,20000003,3f12ca91d24d3ff8eb89058c3ab05433419ea069ed31b754eaf4afa7486f4f8f,0
1MNJYvW6AmRhsrr9ELo5A4mRw2P4yafVZQ,20000111,3f12ca91d24d3ff8eb89058c3ab05433419ea069ed31b754eaf4afa7486f4f8f,1
1HCMAqVqJEYNq444ecHeYJ235soup8wUKb,30000003,3f12ca91d24d3ff8eb89058c3ab05433419ea069ed31b754eaf4afa7486f4f8f,0
1tkf9nKFNqqD5FNyxU6C4CFBtk8dQFFLw,400000003,3f12ca91d24d3ff8eb89058c3ab05433419ea069ed31b754eaf4afa7486f4f8f,0
1NwuhznLyRaQ7e3hWpFveMVimsH5FfdhLa,66666555,3f12ca91d24d3ff8eb89058c3ab05433419ea069ed31b754eaf4afa7486f4f8f,10
