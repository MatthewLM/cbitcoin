package Crypt::PBC::Element;

use strict;
use Carp;
use MIME::Base64;
use Math::BigInt lib => 'GMP';

our %tt; # This maps our element types and our pairings.  Arguably this should be
         # done in the element references themselves, but those are scalar refs, not
         # hash refs.

use overload
    '""'       => sub { my $this = shift; "Crypt::PBC::Element-$tt{$$this}{t}#$$this" },
    'nomethod' => sub { my $this = shift; my $that = pop; croak "arithmetic operation '$that' not defined for $this" };

1;

# DESTROY {{{
sub DESTROY {
    my $this = shift;

    my $i = $$this;

    Crypt::PBC::element_clear( $this );

    delete $tt{$i};
}
# }}}
# clone {{{
sub clone {
    my $this = shift;
    my ($type, $pair) = @{ $tt{$$this} }{qw(t p)};

    my $that = eval "\$pair->init_$type";
    if( $@ ) {
        # Can't call method "init_G1" on an undefined value at (eval 2) line 1.
        # at t/13_pow_arith.t line 28
        chomp $@; $@ =~ s/at \(eval \d+\) line \d+/during Crypt::PBC::Element::clone()/;
        croak $@;
    }

    return $that->set( $this );
}
*copy = *clone;
# }}}

#### exporters
# as_bytes {{{
sub as_bytes {
    my $this = shift;

    return Crypt::PBC::export_element( $this );
}
# }}}
# as_hex {{{
sub as_hex {
    my $this = shift;

    return unpack("H*", $this->as_bytes);
}
*as_str = *as_hex;
# }}}
# as_base64 {{{
sub as_base64 {
    my $this = shift;
    my $arg  = shift || "";

    my $that = encode_base64($this->as_bytes, $arg);
    $that =~ s/\n$//sg;

    return $that;
}
# }}}
# as_bigint {{{
sub as_bigint {
    my $this = shift;
    my $that = Crypt::PBC::element_to_mpz($this);

    my $int = new Math::BigInt;
       $int->{value} = $that;
       $int->{sign}  = '+';

     # I wanted to do something like thits, but I think
     # the mpz_t's returned from element_to_mpz are always going to be positive...
     # $int->{sign}  = $this->is_neg ? "-" : "+";

    return $int;
}
# }}}
# stddump {{{
sub stddump {
    my $this = shift;

    Crypt::PBC::element_fprintf(*STDOUT, '%B', $this );
}
# }}}
# errdump {{{
sub errdump {
    my $this = shift;

    return Crypt::PBC::element_fprintf(*STDERR, '%B', $this );
}
# }}}

#### initializers and set routines
# random {{{
sub random {
    my $this = shift;

    Crypt::PBC::element_random( $this );

    return $this;
}
# }}}
# set_to_bytes {{{
sub set_to_bytes {
    my $this = shift;
    my $data = shift;

    croak "provide something to set the element to" unless defined $data and length $data > 0;
    Crypt::PBC::element_from_bytes($this, $data);

    $this;
}
# }}}
# set_to_hash {{{
sub set_to_hash {
    my $this = shift;
    my $hash = shift;

    croak "provide something to set the element to" unless defined $hash and length $hash > 0;
    #my $type = $tt{$$this}{t};
    #warn " >type=$type; hash=$hash...@_...<\n";
    Crypt::PBC::element_from_hash($this, $hash);
    #warn " <type=$type; hash=$hash...@_...>\n";

    $this;
}
# }}}
# set_to_int {{{
sub set_to_int {
    my $this = shift;
    my $int  = shift;

    croak "int provided ($int) is not acceptable" unless $int =~ m/^\-?[0-9]+\z/s;

    Crypt::PBC::element_set_si($this, $int);

    $this;
}
# }}}
# set_to_bigint {{{
sub set_to_bigint {
    my $this = shift;
    my $int  = shift;

    croak "int provided is not a bigint" unless ref $int and $int->isa("Math::BigInt");

    Crypt::PBC::element_set_mpz($this, $int->{value});

    $this;
}
# }}}
# set {{{
sub set {
    my $this = shift;
    my $that = shift;

    croak "LHS and RHS must be algebraically similar ($tt{$$this}{c} vs $tt{$$that}{c})"
        unless $tt{$$this}{c} eq $tt{$$that}{c};

    Crypt::PBC::element_set($this, $that);

    $this;
}
# }}}
# set0 {{{
sub set0 {
    my $this = shift;
    my $that = shift;

    Crypt::PBC::element_set0($this);

    $this;
}
# }}}
# set1 {{{
sub set1 {
    my $this = shift;
    my $that = shift;

    Crypt::PBC::element_set1($this);

    $this;
}
# }}}

#### comparisons
# is0 {{{
sub is0 {
    my $this = shift;

    croak "LHS should have a type" unless exists $tt{$$this};
    return Crypt::PBC::element_is0( $this );
}
# }}}
# is1 {{{
sub is1 {
    my $this = shift;

    croak "LHS should have a type" unless exists $tt{$$this};
    return Crypt::PBC::element_is1( $this );
}
# }}}
# is_eq {{{
sub is_eq {
    my $this = shift;
    my $that = shift;

    croak "LHS should have a type" unless exists $tt{$$this};
    croak "LHS and RHS must be algebraically similar ($tt{$$this}{c} vs $tt{$$that}{c}) "
        unless $tt{$$this}{c} eq $tt{$$that}{c};

    return not Crypt::PBC::element_cmp( $this, $that ); # returns 0 if they're algebraically similar
}
# }}}
# is_sqr {{{
sub is_sqr {
    my $this = shift;

    croak "LHS should have a type" unless exists $tt{$$this};
    my $type = $tt{$$this}{t};

    return 1 if $type eq "G1";
    return 1 if $type eq "G2";
    return 1 if $type eq "GT";

    return Crypt::PBC::element_is_sqr( $this );
}
# }}}

#### exponentiation
# pow_zn {{{
sub pow_zn {
    my $this = shift;
    my $base = shift;
    my $expo = shift;

    if( defined $base and not defined $expo ) {
        $expo = $base;
        $base = $this;
    }

    croak "LHS should have a type" unless exists $tt{$$this};
    croak "LHS and BASE must be algebraically similar ($tt{$$this}{c} vs $tt{$$base}{c})"
        unless $tt{$$this}{c} eq $tt{$$base}{c};

    croak "EXPO must be of type Zr (not $tt{$$expo}{t})" unless $tt{$$expo}{t} eq "Zr";

    Crypt::PBC::element_pow_zn( $this, $base, $expo );

    $this;
}
# }}}
# pow2_zn {{{
sub pow2_zn {
    my $this = shift;
    my $a1 = shift;
    my $n1 = shift;
    my $a2 = shift;
    my $n2 = shift;

    croak "LHS should have a type" unless exists $tt{$$this};
    croak "LHS and a1 must be algebraically similar ($tt{$$this}{c} vs $tt{$$a1}{c})" unless $tt{$$this}{c} eq $tt{$$a1}{c};
    croak "LHS and a2 must be algebraically similar ($tt{$$this}{c} vs $tt{$$a2}{c})" unless $tt{$$this}{c} eq $tt{$$a2}{c};
    croak "n1 must be of type Zr (not $tt{$$n1}{t})" unless $tt{$$n1}{t} eq "Zr";
    croak "n2 must be of type Zr (not $tt{$$n2}{t})" unless $tt{$$n2}{t} eq "Zr";

    Crypt::PBC::element_pow2_zn( $this, $a1, $n1, $a2, $n2 );

    $this;
}
# }}}
# pow3_zn {{{
sub pow3_zn {
    my $this = shift;
    my $a1 = shift;
    my $n1 = shift;
    my $a2 = shift;
    my $n2 = shift;
    my $a3 = shift;
    my $n3 = shift;

    croak "LHS should have a type" unless exists $tt{$$this};
    croak "LHS and a1 must be algebraically similar ($tt{$$this}{c} vs $tt{$$a1}{c})" unless $tt{$$this}{c} eq $tt{$$a1}{c};
    croak "LHS and a2 must be algebraically similar ($tt{$$this}{c} vs $tt{$$a2}{c})" unless $tt{$$this}{c} eq $tt{$$a2}{c};
    croak "LHS and a3 must be algebraically similar ($tt{$$this}{c} vs $tt{$$a3}{c})" unless $tt{$$this}{c} eq $tt{$$a3}{c};
    croak "n1 must be of type Zr (not $tt{$$n1}{t})" unless $tt{$$n1}{t} eq "Zr";
    croak "n2 must be of type Zr (not $tt{$$n2}{t})" unless $tt{$$n2}{t} eq "Zr";
    croak "n3 must be of type Zr (not $tt{$$n3}{t})" unless $tt{$$n3}{t} eq "Zr";

    Crypt::PBC::element_pow3_zn( $this, $a1, $n1, $a2, $n2, $a3, $n3 );

    $this;
}
# }}}

# pow_bigint {{{
sub pow_bigint {
    my $this = shift;
    my $base = shift;
    my $expo = shift;

    if( defined $base and not defined $expo ) {
        $expo = $base;
        $base = $this;
    }

    croak "EXPO provided is not a bigint" unless ref $expo and $expo->isa("Math::BigInt");

    croak "LHS should have a type" unless exists $tt{$$this};
    croak "LHS and BASE must be algebraically similar ($tt{$$this}{c} vs $tt{$$base}{c})"
        unless exists $tt{$$this} and $tt{$$this}{c} eq $tt{$$base}{c};

    Crypt::PBC::element_pow_mpz( $this, $base, $expo->{value} );

    $this;
}
# }}}
# pow2_bigint {{{
sub pow2_bigint {
    my $this = shift;
    my $a1 = shift;
    my $n1 = shift;
    my $a2 = shift;
    my $n2 = shift;

    croak "n1 provided is not a bigint" unless ref $n1 and $n1->isa("Math::BigInt");
    croak "n2 provided is not a bigint" unless ref $n2 and $n2->isa("Math::BigInt");

    croak "LHS should have a type" unless exists $tt{$$this};
    croak "LHS and a1 must be algebraically similar ($tt{$$this}{c} vs $tt{$$a1}{c})" unless $tt{$$this}{c} eq $tt{$$a1}{c};
    croak "LHS and a2 must be algebraically similar ($tt{$$this}{c} vs $tt{$$a2}{c})" unless $tt{$$this}{c} eq $tt{$$a2}{c};

    Crypt::PBC::element_pow2_mpz( $this, $a1, $n1->{value}, $a2, $n2->{value} );

    $this;
}
# }}}
# pow3_bigint {{{
sub pow3_bigint {
    my $this = shift;
    my $a1 = shift;
    my $n1 = shift;
    my $a2 = shift;
    my $n2 = shift;
    my $a3 = shift;
    my $n3 = shift;

    croak "n1 provided is not a bigint" unless ref $n1 and $n1->isa("Math::BigInt");
    croak "n2 provided is not a bigint" unless ref $n2 and $n2->isa("Math::BigInt");
    croak "n3 provided is not a bigint" unless ref $n3 and $n2->isa("Math::BigInt");

    croak "LHS should have a type" unless exists $tt{$$this};
    croak "LHS and a1 must be algebraically similar ($tt{$$this}{c} vs $tt{$$a1}{c})" unless $tt{$$this}{c} eq $tt{$$a1}{c};
    croak "LHS and a2 must be algebraically similar ($tt{$$this}{c} vs $tt{$$a2}{c})" unless $tt{$$this}{c} eq $tt{$$a2}{c};
    croak "LHS and a3 must be algebraically similar ($tt{$$this}{c} vs $tt{$$a3}{c})" unless $tt{$$this}{c} eq $tt{$$a3}{c};

    Crypt::PBC::element_pow3_mpz( $this, $a1, $n1->{value}, $a2, $n2->{value}, $a3, $n3->{value} );

    $this;
}
# }}}

#### arith
## 1op
# square {{{
sub square {
    my $lhs  = shift;
    my $rhs  = shift;

    if( $rhs ) {
        croak "LHS should have a type" unless exists $tt{$$lhs};
        croak "LHS and RHS must be algebraically similar ($tt{$$lhs}{c} vs $tt{$$rhs}{c})" unless $tt{$$lhs}{c} eq $tt{$$rhs}{c};

    } else {
        $rhs = $lhs;
    }

    Crypt::PBC::element_square( $lhs, $rhs );

    $lhs;
}
# }}}
# double {{{
sub double {
    my $lhs  = shift;
    my $rhs  = shift;

    if( $rhs ) {
        croak "LHS should have a type" unless exists $tt{$$lhs};
        croak "LHS and RHS must be algebraically similar ($tt{$$lhs}{c} vs $tt{$$rhs}{c})" unless $tt{$$lhs}{c} eq $tt{$$rhs}{c};

    } else {
        $rhs = $lhs;
    }

    Crypt::PBC::element_double( $lhs, $rhs );

    $lhs;
}
# }}}
# halve {{{
sub halve {
    my $lhs  = shift;
    my $rhs  = shift;

    if( $rhs ) {
        croak "LHS should have a type" unless exists $tt{$$lhs};
        croak "LHS and RHS must be algebraically similar ($tt{$$lhs}{c} vs $tt{$$rhs}{c})" unless $tt{$$lhs}{c} eq $tt{$$rhs}{c};

    } else {
        $rhs = $lhs;
    }

    Crypt::PBC::element_halve( $lhs, $rhs );

    $lhs;
}
# }}}
# neg {{{
sub neg {
    my $lhs  = shift;
    my $rhs  = shift;

    if( $rhs ) {
        croak "LHS should have a type" unless exists $tt{$$lhs};
        croak "LHS and RHS must be algebraically similar ($tt{$$lhs}{c} vs $tt{$$rhs}{c})" unless $tt{$$lhs}{c} eq $tt{$$rhs}{c};

    } else {
        $rhs = $lhs;
    }

    Crypt::PBC::element_neg( $lhs, $rhs );

    $lhs;
}
# }}}
# invert {{{
sub invert {
    my $lhs  = shift;
    my $rhs  = shift;

    if( $rhs ) {
        croak "LHS should have a type" unless exists $tt{$$lhs};
        croak "LHS and RHS must be algebraically similar ($tt{$$lhs}{c} vs $tt{$$rhs}{c})" unless $tt{$$lhs}{c} eq $tt{$$rhs}{c};

    } else {
        $rhs = $lhs;
    }

    Crypt::PBC::element_invert( $lhs, $rhs );

    $lhs;
}
# }}}

## 2op
# add {{{
sub add {
    my $lhs  = shift;
    my $rhs1 = shift;
    my $rhs2 = shift;

    if( $rhs2 ) {
        croak "LHS should have a type" unless exists $tt{$$lhs};
        croak "LHS, RHS1 and RHS2 must be algebraically similar ($tt{$$lhs}{c} vs $tt{$$rhs1}{c} vs $tt{$$rhs2}{c})"
            unless $tt{$$lhs}{c} eq $tt{$$rhs1}{c} and $tt{$$rhs1}{c} eq $tt{$$rhs2}{c};

        Crypt::PBC::element_add( $lhs, $rhs1, $rhs2 );

    } else {
        croak "LHS should have a type" unless exists $tt{$$lhs};
        croak "LHS and RHS should be algebraically similar ($tt{$$lhs}{c} vs $tt{$$rhs1}{c})"
            unless $tt{$$lhs}{c} eq $tt{$$rhs1}{c};

        Crypt::PBC::element_add( $lhs, $lhs, $rhs1 );
    }

    $lhs;
}
# }}}
# Sub {{{
sub Sub {
    my $lhs  = shift;
    my $rhs1 = shift;
    my $rhs2 = shift;

    if( $rhs2 ) {
        croak "LHS should have a type" unless exists $tt{$$lhs};
        croak "LHS, RHS1 and RHS2 must be algebraically similar ($tt{$$lhs}{c} vs $tt{$$rhs1}{c} vs $tt{$$rhs2}{c})"
            unless $tt{$$lhs}{c} eq $tt{$$rhs1}{c} and $tt{$$rhs1}{c} eq $tt{$$rhs2}{c};

        Crypt::PBC::element_sub( $lhs, $rhs1, $rhs2 );

    } else {
        croak "LHS should have a type" unless exists $tt{$$lhs};
        croak "LHS and RHS should be algebraically similar ($tt{$$lhs}{c} vs $tt{$$rhs1}{c})"
            unless $tt{$$lhs}{c} eq $tt{$$rhs1}{c};

        Crypt::PBC::element_sub( $lhs, $lhs, $rhs1 );
    }

    $lhs;
}
# }}}
# mul {{{
sub mul {
    my $lhs  = shift;
    my $rhs1 = shift;
    my $rhs2 = shift;

    if( $rhs2 ) {
        croak "LHS should have a type" unless exists $tt{$$lhs};
        croak "LHS, RHS1 and RHS2 must be algebraically similar ($tt{$$lhs}{c} vs $tt{$$rhs1}{c} vs $tt{$$rhs2}{c})"
            unless $tt{$$lhs}{c} eq $tt{$$rhs1}{c} and $tt{$$rhs1}{c} eq $tt{$$rhs2}{c};

        Crypt::PBC::element_mul( $lhs, $rhs1, $rhs2 );

    } else {
        croak "LHS should have a type" unless exists $tt{$$lhs};
        croak "LHS and RHS should be algebraically similar ($tt{$$lhs}{c} vs $tt{$$rhs1}{c})"
            unless $tt{$$lhs}{c} eq $tt{$$rhs1}{c};

        Crypt::PBC::element_mul( $lhs, $lhs, $rhs1 );
    }

    $lhs;
}
# }}}
# div {{{
sub div {
    my $lhs  = shift;
    my $rhs1 = shift;
    my $rhs2 = shift;

    if( $rhs2 ) {
        croak "LHS should have a type" unless exists $tt{$$lhs};
        croak "LHS, RHS1 and RHS2 must be algebraically similar ($tt{$$lhs}{c} vs $tt{$$rhs1}{c} vs $tt{$$rhs2}{c})"
            unless $tt{$$lhs}{c} eq $tt{$$rhs1}{c} and $tt{$$rhs1}{c} eq $tt{$$rhs2}{c};

        Crypt::PBC::element_div( $lhs, $rhs1, $rhs2 );

    } else {
        croak "LHS should have a type" unless exists $tt{$$lhs};
        croak "LHS and RHS should be algebraically similar ($tt{$$lhs}{c} vs $tt{$$rhs1}{c})"
            unless $tt{$$lhs}{c} eq $tt{$$rhs1}{c};

        Crypt::PBC::element_div( $lhs, $lhs, $rhs1 );
    }

    $lhs;
}
# }}}

# mul_zn {{{
sub mul_zn {
    my $lhs  = shift;
    my $rhs1 = shift;
    my $rhs2 = shift;

    if( $rhs2 ) {
        croak "LHS should have a type" unless exists $tt{$$lhs};
        croak "LHS, RHS1 must be algebraically similar ($tt{$$lhs}{c} vs $tt{$$rhs1}{c})"
            unless $tt{$$lhs}{c} eq $tt{$$rhs1}{c};
        croak "RHS2 should be in Zr (not $tt{$$rhs2}{t})" unless $tt{$$rhs2}{t} eq "Zr";

        Crypt::PBC::element_mul_zn( $lhs, $rhs1, $rhs2 );

    } else {
        croak "RHS should be in Zr (not $tt{$$rhs1}{t})"
            unless $tt{$$rhs1}{t} eq "Zr";

        Crypt::PBC::element_mul_zn( $lhs, $lhs, $rhs1 );
    }

    $lhs;
}
# }}}
# mul_int {{{
sub mul_int {
    my $lhs  = shift;
    my $rhs1 = shift;
    my $rhs2 = shift;

    if( $rhs2 ) {
        croak "LHS should have a type" unless exists $tt{$$lhs};
        croak "LHS, RHS1 must be algebraically similar ($tt{$$lhs}{c} vs $tt{$$rhs1}{c})"
            unless $tt{$$lhs}{c} eq $tt{$$rhs1}{c};

        croak "int provided ($rhs2) is not acceptable" unless $rhs2 =~ m/^\-?[0-9]+\z/s;

        Crypt::PBC::element_mul_si( $lhs, $rhs1, $rhs2 );

    } else {
        croak "int provided ($rhs1) is not acceptable" unless $rhs1 =~ m/^\-?[0-9]+\z/s;

        Crypt::PBC::element_mul_si( $lhs, $lhs, $rhs1 );
    }

    $lhs;
}
# }}}
# mul_bigint {{{
sub mul_bigint {
    my $lhs  = shift;
    my $rhs1 = shift;
    my $rhs2 = shift;

    if( $rhs2 ) {
        croak "LHS should have a type" unless exists $tt{$$lhs};
        croak "LHS, RHS1 must be algebraically similar ($tt{$$lhs}{c} vs $tt{$$rhs1}{c})"
            unless $tt{$$lhs}{c} eq $tt{$$rhs1}{c};

        croak "int provided is not a bigint" unless ref $rhs2 and $rhs2->isa("Math::BigInt");

        Crypt::PBC::element_mul_si( $lhs, $rhs1, $rhs2 );

    } else {
        croak "int provided is not a bigint" unless ref $rhs1 and $rhs1->isa("Math::BigInt");

        Crypt::PBC::element_mul_si( $lhs, $lhs, $rhs1 );
    }

    $lhs;
}
# }}}

# pairing_apply {{{
sub pairing_apply {
    my $this = shift;
    my $rhs1 = shift;
    my $rhs2 = shift;
    my $pair = $tt{$$this}{p};

    my $c1 = $tt{$$rhs1}{c};
    my $c2 = $tt{$$rhs2}{c};

    croak "group type for LHS must be GT (not $tt{$$this}{t})"  unless $tt{$$this}{t} eq "GT";
    croak "group type for RHS1 must be G1 (not $c1)" unless $c1 eq "G1" or $c1 eq "G[12]";
    croak "group type for RHS2 must be G2 (not $c2)" unless $c2 eq "G2" or $c2 eq "G[12]";

    Crypt::PBC::pairing_apply( $this => ($rhs1, $rhs2) => $pair );

    $this;
}
*ehat          = *pairing_apply;
*e_hat         = *pairing_apply;
*apply_pairing = *pairing_apply;
# }}}

#### package Crypt::PBC::Pairing {{{

package Crypt::PBC::Pairing;

use strict;
use Carp;

1;

sub _stype {
    my $this = shift;
    my $that = shift;
    my $type = shift;

    $Crypt::PBC::Element::tt{$$that} = {
        t => $type,
        p => $this,
        c => $type,
    };

    if( $type =~ m/G[12]/ and Crypt::PBC::pairing_is_symmetric($this) ) {
        $Crypt::PBC::Element::tt{$$that}{c} = "G[12]";
    }

    return;
}

sub init_G1 { my $this = shift; my $that = Crypt::PBC::element_init_G1( $this ); $this->_stype($that => "G1"); $that }
sub init_G2 { my $this = shift; my $that = Crypt::PBC::element_init_G2( $this ); $this->_stype($that => "G2"); $that }
sub init_GT { my $this = shift; my $that = Crypt::PBC::element_init_GT( $this ); $this->_stype($that => "GT"); $that }
sub init_Zr { my $this = shift; my $that = Crypt::PBC::element_init_Zr( $this ); $this->_stype($that => "Zr"); $that }
sub DESTROY { my $this = shift; my $that = Crypt::PBC::pairing_clear(   $this ); }

# }}}
#### package Crypt::PBC {{{

package Crypt::PBC;

use strict;
use warnings;
use Carp;

our $VERSION = '0.9000';

#    use base 'Exporter';
#    our %EXPORT_TAGS = ( 'all' => [ qw( ) ] );
#    our @EXPORT_OK = ( @{ $EXPORT_TAGS{'all'} } );
#    our @EXPORT = qw( );
#
#    sub AUTOLOAD {
#        my $constname;
#        our $AUTOLOAD; ($constname = $AUTOLOAD) =~ s/.*:://;
#        croak "Crypt::PBC::constant wtf($AUTOLOAD, $constname) not defined" if $constname eq 'constant';
#        my ($error, $val) = constant($constname);
#        if( $error ) { croak $error }
#        goto &$AUTOLOAD;
#    }

require XSLoader;
XSLoader::load('Crypt::PBC', $VERSION);

1;

sub new {
    my $class = shift;
    my $that;
    my $arg = shift;

    TOP: {
        if( ref($arg) eq "GLOB" ) {
            my $contents = do { local $/; <$arg> };

            $arg = $contents;
            redo TOP;

        } elsif( $arg !~ m/\n/ and -f $arg ) {
            open my $in, $arg or croak "couldn't open param file ($arg): $!";
            my $contents = do { local $/; <$in> }; close $in;

            $arg = $contents;
            redo TOP;

        } elsif( $arg ) {
            $arg =~ s/^\s*//s;
            $arg =~ s/\s*$//s;

            if( $arg =~ m/^(?s:type\s+[a-z]+\s*|[a-z0-9]+\s+[0-9]+\s*)+\z/s ) {
                $that = Crypt::PBC::pairing_init_str($arg);

            } else {
                croak "either the filename doesn't exist or that param string is unparsable: $arg";
            }

        } else {
            croak "you must pass a file, glob (stream), or init params to new()";
        }
    }

    croak "something went wrong ... you must pass a file, glob (stream), or init params to new()" unless $$that>0;
    return $that;
}

# }}}

1;
