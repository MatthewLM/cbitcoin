
use strict;
use Test;
use Carp;
use Crypt::PBC;

if( defined $ENV{SKIP_ALL_BUT} ) { unless( $0 =~ m/\Q$ENV{SKIP_ALL_BUT}\E/ ) { plan tests => 1; skip(1); exit 0; } }

my $curve = new Crypt::PBC("params_d.txt");
my @e = ( $curve->init_G1, $curve->init_G2, $curve->init_GT, $curve->init_Zr, 1, new Math::BigInt(19) );
my @i = ( 0 .. $#e ); # the indicies for permute()

if( -f "slamtest.log" ) {
    unlink "slamtest.log" or die "couldn't remove old logfile: $!";
}

my %slam_these = (
    pairing_apply => 2,

    random => 1, # technically these should be 0, but this test is not set up for no-args
    square => 1,
    double => 1,
    halve  => 1,
    neg    => 1,
    invert => 1,

    add => 2,
    Sub => 2,
    mul => 2,
    div => 2,

    mul_zn     => 2,
    mul_int    => 2,
    mul_bigint => 2,

    pow_zn  => 2,
    pow2_zn => 4,
    pow3_zn => 6,

    pow_bigint  => 2,
    pow2_bigint => 4,
    pow3_bigint => 6,

    is0    => 1,
    is1    => 1,
    is_eq  => 1,
    is_sqr => 1,

    set0   => 1,
    set1   => 1,

    set_to_hash   => 1,
    set_to_bytes  => 1,
    set_to_int    => 1,
    set_to_bigint => 1,
    set           => 1,
);

#### This test may need some explaining... We wish to pass all
#### possible all the wrong things and make sure we catch all the
#### potential sagfaults with perl croak() errors.

plan tests => int keys %slam_these;

my %huge_cache = ();

my $start_time = time;
my $total_per  = 0;
my $last_time  = 0;

$ENV{MAX_PERM_TIME} = 0.05 unless defined $ENV{MAX_PERM_TIME} and $ENV{MAX_PERM_TIME} >= 0;
warn "\n\t$0 is set to truncate all tests longer than $ENV{MAX_PERM_TIME} second(s) (env MAX_PERM_TIME)\n" if $ENV{MAX_PERM_TIME} < 120;
eval 'use Time::HiRes qw(time)'; # does't matter if this fails...
warn "\t$0 gives more accurate calls/s estimates if Time::HiRes is installed...\n" if $@;

my $shh = $ENV{MAX_PERM_TIME} < 15;

for my $function (sort slam_sort keys %slam_these) {
    my @a = &permute( $slam_these{$function} => @i );

    # warn " [31mWARN($function, " . (int @a) . ")[0m";

    if( $total_per > 0 and (my $delta_t = time - $start_time) > 0 ) {
        my $v = "";
           $v = ($delta_t / $total_per);
        my $t = ($v >= 1 ? sprintf('%0.2f s/call', $v) : sprintf('%0.2f calls/s', 1/$v));

        my $m = int @a;
        if( my $total = ($v * $m) > $ENV{MAX_PERM_TIME} ) {
            my $mpti = int ($ENV{MAX_PERM_TIME}/$v);
               $mpti = 1 if $mpti < 1;

            @a = sort { (rand 1) <=> (rand 1) } @a;
            @a = @a[ 0 .. $mpti ];

            my $nc = int @a;

            $m = "$nc (reduced randomly from $m)";
        }
         
        unless( $shh ) {
            warn " testing $m argument permutations for $function() $t\n" if $last_time != time;
        }
        $last_time = time;
    }

    for my $a (@a) {
        my $key = "@$a";
        my $args = $huge_cache{$key};
           $args = [map { ( ref $e[$_] and $e[$_]->isa("Crypt::PBC::Element") ? $e[$_]->clone->random : $e[$_]) } @$a]
               if not defined $args;
        $huge_cache{$key} = $args;

        for my $e (@e) {
            next unless ref $e and $e->isa("Crypt::PBC::Element");

            ## DEBUG ## open OUTPUT, ">>slamtest.log" or die $!;
            ## DEBUG ## print OUTPUT "e=$e; function=$function; args=[@$args];\n";
            ## DEBUG ## close OUTPUT;

            eval '$e->random->' . $function . '(@$args)';

            # We are just looking for segmentation faults for now
            # so we ignore most $@ entirely.

            if( $@ and not $@ =~ m/(?:SCALAR ref|HASH ref|provide something|same group|int.provided.*accept|RHS|LHS|is not a bigint|must be.*(?:G1|G2|GT|Zr))/ ) {
                open OUTPUT, ">>slamtest.log" or die $!;
                warn " [logged] \$@=$@";
                print OUTPUT " function=$function; \$@=$@";
                close OUTPUT;
            }
        }
    }

    $total_per += (int @a);

    ok( 1 );
}

# _permute {{{
sub _permute {
    my $num = shift;
    my $arr = shift;
    my $src = shift;

    unshift @$_, $src->[0] for @$arr;

    my $e = $#{ $arr };
    for my $i (1 .. $#$src) {
        for my $j (0 .. $e) {
            my $t = [@{ $arr->[$j] }];
            
            $t->[0] = $i;

            push @$arr, $t;
        }
    }

    &_permute( $num-1, $arr, $src ) if $num > 1;
}
# }}}
# permute {{{
sub permute {
    my $anum = shift; croak "dumb number" unless $anum > 0;
    my @ret = ();

    for my $num ( 1 .. $anum ) {
        my @a = map {[$_]} @_;

        &_permute( $num-1, \@a, \@_ ) if $num > 1;

        push @ret, @a;
    }

    return @ret;
}
# }}}
# slam_sort {{{
sub slam_sort {
    my ($c, $d) = ($slam_these{$a}, $slam_these{$b});

    return $c <=> $d if $c != $d;
    return $a cmp $b;
}
# }}}
