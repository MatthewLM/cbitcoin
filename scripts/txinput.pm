 use strict;
 use warnings;
 use DBI;
 use Digest::SHA qw(sha256);
 use CBitcoin::Script;
 use CBitcoin::TransactionInput;
 use CBitcoin::TransactionOutput;
print "hello\n";

for (<DATA>) {
	next if /^#/;
	chomp;
	eval {
		use bigint;
		my ($address,$value,$prevOutHash,$prevOutIndex) = split(',',$_);

		my $script = CBitcoin::Script::address_to_script($address);
		#my $newaddress = CBitcoin::Script::script_to_address($script);
		#my $out = CBitcoin::TransactionOutput->new({'script' => $script, 'value' => $value});
		my $in = CBitcoin::TransactionInput->new({
			'prevOutHash' => $prevOutHash
			,'prevOutIndex' => $prevOutIndex
			,'script' => $script
		});
					

		my $bool = 0;
		if(
			$prevOutHash eq $in->prevOutHash()
			&& $prevOutIndex eq $in->prevOutIndex()
			&& $script eq $in->script()
		){
			$bool = 1;
		}


#		$bool = 1 if $script eq $out->script() && $value eq $out->value() && $address eq CBitcoin::Script::script_to_address($out->script());
		
		print "Test:($script|$bool)\n";
	};
	if($@){ warn "Error:$@";}
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
