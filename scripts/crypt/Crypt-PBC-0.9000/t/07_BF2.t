# vi:fdm=marker fdl=0 syntax=perl:

use strict;
use Test;

if( defined $ENV{SKIP_ALL_BUT} ) { unless( $0 =~ m/\Q$ENV{SKIP_ALL_BUT}\E/ ) { plan tests => 1; skip(1); exit 0; } }

my $bf = 0;
my $sh = 0;
eval q{
    use Crypt::CBC;
    use Crypt::Blowfish;

    $bf = 1;
};

eval q{
    use Digest::SHA1 qw(sha1);

    $sh = 1;
};

plan tests => 3 + $bf;

use Crypt::PBC;

# SETUP

my $curve = Crypt::PBC->new("params_d.txt");

my $P     = $curve->init_G2->random; # generator in G1 -- even though it's in G2
my $s     = $curve->init_Zr->random; # master secret
my $P_pub = $curve->init_G2->pow_zn( $P, $s ); # master public key

# EXTRACT

my $Q_id = $curve->init_G1;
if( $sh ) {
    warn "using Digest::SHA1 to generate Q_id\n" if $ENV{EXTRA_INFO};
    $Q_id->set_to_hash( sha1("Paul Miller <jettero a gmail or cpan> | expires 2007-11-15") );

} else {
    $Q_id->random; # this is just a test anyway
}
my $d_id = $curve->init_G1->pow_zn( $Q_id, $s );

# ENCRYPT

my $r    = $curve->init_Zr->random;
my $g_id = $curve->init_GT->e_hat( $Q_id, $P_pub );
my $U    = $curve->init_G2->pow_zn( $P, $r ); # U is the part d_id can use to derive w
my $w    = $curve->init_GT->pow_zn( $g_id, $r ); # w is the part you'd xor(w,M) to get V or xor(w,V) to get M

# DECRYPT
my $w_from_U = $curve->init_GT->e_hat( $d_id, $U );

ok( $w_from_U->is_eq( $w ) );
ok( $w_from_U->as_bytes, $w->as_bytes ); # binary good
ok( $w_from_U->as_str,   $w->as_str   ); # hexidecimal

if( $bf ) {
    # If the three comparisons above worked, this is kindof a no-brainer; but,
    # personally, I was confused on how to M^H2(g^r) -- and here it is:

    my $cipher1 = new Crypt::CBC({header=>"randomiv", key=>$w->as_bytes,        cipher=>'Blowfish'});
    my $cipher2 = new Crypt::CBC({header=>"randomiv", key=>$w_from_U->as_bytes, cipher=>'Blowfish'});
    my $message = "Holy smokes, this is secret!!";
    my $encrypt = $cipher1->encrypt($message);
    my $decrypt = $cipher2->decrypt($encrypt);

    warn " using Crypt::CBC(Crypt::Blowfish) for 4th test\n" if $ENV{EXTRA_INFO};
    ok( $decrypt, $message );
}
