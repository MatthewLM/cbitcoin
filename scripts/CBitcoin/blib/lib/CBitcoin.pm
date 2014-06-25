package CBitcoin;

use 5.014002;
use strict;
use warnings;

use CBitcoin::CBHD;

require Exporter;

our @ISA = qw(Exporter);

# Items to export into callers namespace by default. Note: do not export
# names by default without a very good reason. Use EXPORT_OK instead.
# Do not simply export all your public functions/methods/constants.

# This allows declaration	use CBitcoin ':all';
# If you do not need this, moving things directly into @EXPORT or @EXPORT_OK
# will save memory.
our %EXPORT_TAGS = ( 'all' => [ qw(
	
) ] );

our @EXPORT_OK = ( @{ $EXPORT_TAGS{'all'} } );

our @EXPORT = qw(
	
);

our $VERSION = '0.01';


# Preloaded methods go here.
sub new {
	my $package = shift;
	return bless({}, $package);
}
# newMasterKey deriveChildPrivate exportWIFFromCBHDKey exportAddressFromCBHDKey publickeyFromWIF
# generate a key (parent)
sub generate {
	my $this = shift;
	eval{
		my $key = CBitcoin::CBHD::newMasterKey(1);
		$this->load($key) || die "Cannot load the key.";		
	};
	if($@){
		return 0;
	}
	return 1;
}
# load
sub load {
	my $this = shift;
	my $serializedkey = shift;
	
	eval{
		# TODO:we need to do some eval here to see if this is a legit master key
		# .. test(key) || die "key is bad";
		$this->{serializedkey} = $serializedkey;
	};
	if($@){
		return 0;
	}
	return 1;
}


sub deriveChild {
	my $this = shift;
	my $hardbool = shift;
	my $childid = shift;
	my $childkey = new CBitcoin;
	eval{
		if($hardbool){
			$hardbool = 1;
		}
		else{
			$hardbool = 0;
		}
		unless($childid > 0 && $childid < 2**31){
			die "The child id is not in the correct range.\n";
		}
		$childkey->load(CBitcoin::CBHD::deriveChildPrivate($hardbool,$childid));
		
	};
	if($@){
		return undef;
	}
	return $childkey;
	
}

sub WIF {
	my $this = shift;
	my $wif = '';
	eval{
		die "no internal key" unless $this->{serializedkey} ;
		$wif = CBitcoin::CBHD::exportWIFFromCBHDKey($this->{serializedkey});
	};
	if($@){
		return undef;
	}
	return $wif;
}

sub address {
	my $this = shift;
	my $address = '';
	eval{
		$address = CBitcoin::CBHD::exportAddressFromCBHDKey
	};
	if($@){
		return undef;
	}
}

1;
__END__
# Below is stub documentation for your module. You'd better edit it!

=head1 NAME

CBitcoin - Perl extension for the cbitcoin library

=head1 SYNOPSIS

  use CBitcoin;

  my $parentkey = new CBitcoin;
  $parentkey->generate();
  my $childkey = $parentkey->deriveChild(0,333222);
  my $wifstring = $childkey->WIF();
  my $address = $childkey->Address();
  
 

=head1 DESCRIPTION

This module was built, in order to perform bitcoin cryptographic operations on low powered devices. 

=head2 EXPORT





=head1 SEE ALSO

Please see the C library cbitcoin for documentation on how the c functions work.

=head1 AUTHOR

Joel DeJesus, E<lt>dejesus.joel@e-flamingo.jp<gt>

=head1 COPYRIGHT AND LICENSE

Copyright (C) 2014 by Joel DeJesus

This library is free software; you can redistribute it and/or modify
it under the same terms as Perl itself, either Perl version 5.14.2 or,
at your option, any later version of Perl 5 you may have available.


=cut
