## This file generated by InlineX::C2XS (version 0.22) using Inline::C (version 0.5)
package CBitcoin::TransactionInput;



require Exporter;
*import = \&Exporter::import;
require DynaLoader;

$CBitcoin::TransactionInput::VERSION = '0.01';

DynaLoader::bootstrap CBitcoin::TransactionInput $CBitcoin::TransactionInput::VERSION;

@CBitcoin::TransactionInput::EXPORT = ();
@CBitcoin::TransactionInput::EXPORT_OK = ();

sub dl_load_flags {0} # Prevent DynaLoader from complaining and croaking

{ ## start of bigint block
	use bigint;

	sub new {
		my $package = shift;
		my $this = bless({}, $package);
	
		my $x = shift;
		unless(ref($x) eq 'HASH'){
			return $this;
		}
		if(defined $x->{'data'}){
			# we have a tx input which is serialized
			$this->importSerializedData($x->{'data'});
		
		}
		elsif(defined $x->{'prevOutHash'} 
			&& $x->{'prevOutIndex'} =~ m/[0-9]+/
			&& defined $x->{'script'}
		){
			# we have the data, let's get the serialized data
			$this->prevOutHash($x->{'prevOutHash'});
			$this->prevOutIndex($x->{'prevOutIndex'});
			$this->script($x->{'script'});
			$this->sequence(hex('0xFFFFFFFF')) unless defined $this->sequence();
			# call this function to validate the data, and get serialized data back
			$this->deserializeData();
		}
	
	
		return $this;
	}

	sub prevOutHash {
		my $this = shift;
		die "not correct TransactionInput type" unless ref($this) eq 'CBitcoin::TransactionInput';
		my $x = shift;
		if($x){
			# TODO: do input validation here
			$this->{prevOutHash} = $x;
			return $x;
		}
		else{
			return $this->{prevOutHash};
		}
	}

	sub prevOutIndex {
		my $this = shift;
		die "not correct TransactionInput type" unless ref($this) eq 'CBitcoin::TransactionInput';
		my $x = shift;
		if($x){
			# make sure that this is a positive integer
			if($x =~ m/[0-9]+/){
				$this->{prevOutIndex} = $x;
				return $x;
			}
			else{
				return undef;
			}
		}
		else{
			return $this->{prevOutIndex};
		}	
	}

	sub script {
		my $this = shift;
		die "not correct TransactionInput type" unless ref($this) eq 'CBitcoin::TransactionInput';
		my $x = shift;
		if($x){
			# TODO
			$this->{script} = $x;
			return $x;
		}
		else{
			return $this->{script};
		}	
	}


	sub sequence {
		my $this = shift;
		die "not correct TransactionInput type" unless ref($this) eq 'CBitcoin::TransactionInput';
		my $x = shift;
		if($x){
			# TODO
			$this->{sequence} = $x;
			return $x;
		}
		else{
			return $this->{sequence};
		}	
	}

	sub serializeddata {
		my $this = shift;
		die "not correct TransactionInput type" unless ref($this) eq 'CBitcoin::TransactionInput';
		my $x = shift;
		if($x){
			# TODO
			$this->{serializeddata} = $x;
			return $x;
		}
		else{
			return $this->{serializeddata};
		}	
	}


	sub deserializeData {
		my $this = shift;
		die "not correct TransactionInput type" unless ref($this) eq 'CBitcoin::TransactionInput';
		$this->serializeddata( CBitcoin::TransactionInput::create_txinput_obj($this->script(), 
			$this->sequence(), 
			$this->prevOutHash(),
			$this->prevOutIndex()
		));
		if(defined $this->serializeddata()){
			return 1;
		}
		else{
			return 0;
		}
	}
=pod

---++ importSerializedData

Call this when you want the serialized data to be split up, parsed and assigned to other private variables.

=cut
	sub importSerializedData {
		my $this = shift;
		die "not correct TransactionInput type" unless ref($this) eq 'CBitcoin::TransactionInput';
	
		my $x = shift;
	
		if( $this->script(CBitcoin::TransactionInput::get_script_from_obj($x))
			&& $this->prevOutHash(CBitcoin::TransactionInput::get_prevOutHash_from_obj($x)   )
			&& $this->prevOutIndex(CBitcoin::TransactionInput::get_prevOutIndex_from_obj($x)   )
			&& $this->sequence(CBitcoin::TransactionInput::get_sequence_from_obj($x)   )
			){
			$this->{serializeddata} = $x;
			return 1;		
		}
		else{
			return undef;
		}
	}

} # end of bigint block
1;
