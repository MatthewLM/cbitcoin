 use strict;
 use warnings;
 use DBI;
 use Digest::SHA qw(sha256);
 use CBitcoin::Script;
 use CBitcoin::TransactionInput;
print "hello\n";

my $address = "1Fjf3xLuuQpuyw6EyxkXuLHzJUw7yUwPTi";
print "Address:$address\n";
my $script = CBitcoin::Script::address_to_script($address);
print "Script:$script\n";
$address = CBitcoin::Script::script_to_address($script);
print "Address:$address\n";
for (<DATA>) {
	next if /^#/;
	chomp;
	eval {
		my ($address,$blah) = split(',',$_);
		print "Address:[$address]\n";
		my $script = CBitcoin::Script::address_to_script($address);
		my $newaddress = CBitcoin::Script::script_to_address($script);
		my $bool = 0;
		$bool = 1 if $address eq $newaddress;
		print "Test:($script|$bool)\n";
	};
	if($@){ warn "Error:$@";}
}
__DATA__
# The following keys were created using javascript code on bitaddress.org
16PRgUZvCneM7AYJ94TaoGE4rMQnRdqqt4,5JkYWudPXkXHkmayfUz52WsweFpY7saT4V9vh4ZNtWgFLLwNgej
1KDxAxej4NZMQtao9xZGiadbsqxcKJt9Ng,5J4G4vaBCiAF881AMEEyc8uC2EHgLfCFP4BL97EvD5vnH99yC6p
1PnA88ck7hGSsSqpPXhaVbWL3suWXEqfsF,5KKGiz5ViCpSXzWCm9ff48g5AdK54FR3w1ByrhQDb1U6kgjmgr2
12jXM28Awqgm2NPgiD6EVjZmih66U5mUAt,5JfTp8uzBFzcJWhjrb7wJfevehwjT6c3WCBcYivpoRcjaSAgHtZ
1BV1GXQmBKF6v6CqUjH6y95KNXqtCoLNWH,5JGcHNTEqrYGVuE2sRw2Ys26xY51ypm4c1dnfZGyw8naAUjkdGM
1AsCoc5UVkHcfFRXUA2YtDJbse7k6Ws4Pz,5J3fMYfP7knUYk67eETrQcby9vhUHjdH9mz5Veag4TvuhX9JMnF
1MNJYvW6AmRhsrr9ELo5A4mRw2P4yafVZQ,5JEwVA4ek7Kcm23oPKaJEx6dj9G9UcM6DCybSb3hhaG8875L3hb
1HCMAqVqJEYNq444ecHeYJ235soup8wUKb,5KVZt4CfaBhPHaJ9Bw9b9zoM7TfnPBTwz3SyYBcn2fEdwHfrzJF
1tkf9nKFNqqD5FNyxU6C4CFBtk8dQFFLw,5JQvrDNPZeLN78jYPpscv3x15pgqDDdkMBwjzva3PMCBagop2yA
1NwuhznLyRaQ7e3hWpFveMVimsH5FfdhLa,5K2dPvJfj2MUvfi5zGxPvUupqPVR2o6PqEtsHzfVPregUoYKUBE
