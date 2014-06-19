use strict;
use lib qw(inc);
use Devel::CheckLib;
use ExtUtils::MakeMaker;

my @libpath = qw(/lib /usr/lib /usr/local/lib);
my @incpath = qw(/usr/include/pbc /usr/local/include/pbc /usr/include /usr/local/include);

my $ver    = "0.5.1";
my @extra  = (
    LIBS => ['-lpbc'],
    INC  => join(" ", map {"-I$_"} (@incpath)),
);

eval { assert_lib( lib => 'pbc', libpath=>\@libpath, incpath=>\@incpath, header => "pbc.h" ); };
if( $@ ) {
    warn "Error locating libpbc version 0.5.1: $@";

    warn "\nYou can get the latest debian binaries from https://voltar.org/pbcfiles\n\n";
    sleep 1;

    my $res = ($ENV{BUILD_HERE} || prompt("Would you like to download and build libpbc from tarball automatically? [y/N]", "n"));
    if( $res =~ m/y/i ) {
        exit 0 unless grab_libpbc($ver);
        exit 0 unless unpack_libpbc($ver);
        exit 0 unless build_libpbc($ver);

    } else {
        exit 0;
    }
}

WriteMakefile(
    NAME              => 'Crypt::PBC',
    VERSION_FROM      => 'PBC.pm',
    PREREQ_PM         => { 
        'MIME::Base64'      => 0,
        'Math::BigInt::GMP' => 0,
        'Math::BigInt'      => 0,
    },

    ($] >= 5.005 ?
      (ABSTRACT_FROM  => 'lib/Crypt/PBC.pod',
       AUTHOR         => 'Paul Miller <jettero@cpan.org>') : ()),

    ($ExtUtils::MakeMaker::VERSION ge '6.48'? 
        (MIN_PERL_VERSION => 5.006001,
            META_MERGE => {
                keywords => [qw(pbc ecc crypt boneh ibe)],
                resources=> {
                    repository => 'http://github.com/jettero/crypt--pbc/',
                },
            },

        LICENSE	=> 'LGPL',
    ) : ()),

    @extra,

    clean  => { FILES => "libpbc.a .pbctest pbc-$ver.tar.gz pbc-$ver slamtest.log " . join(" ", grep {s/\.c$//} <*.c>) },
    depend => {
        "PBC.c" => " earith.xs ecomp.xs einit.xs pairing.xs ",
    },

);

sub build_libpbc {
    my $ver = shift;

	@extra = (
        MYEXTLIB => "libpbc.a",
        INC      => "-Ipbc-$ver/include",
        LIBS     => "-lgmp",
    );

    warn "(You do need libgmp installed in order for this PBC.so to function...)\n";
    check_lib_or_exit( lib => 'gmp', libpath=>\@libpath, incpath=>\@incpath, header=>"gmp.h" );
    warn "(... it seems you have libgmp.  Nevermind.)\n";

    # we don't literally build it, we add things to the makefile
    *MY::postamble = \&postamble;
}

sub postamble {
    my @CFLAGS = map {"-I$_"} @incpath;
    my @LFLAGS = map {"-L$_"} @libpath;

"
PBC\$(OBJ_EXT) : \$(MYEXTLIB)

\$(MYEXTLIB): pbc-$ver/.libs/libpbc.so
	\$(AR) \$(AR_STATIC_ARGS) \$@ pbc-$ver/.libs/*.o

pbc-$ver/.libs/libpbc.so:
	cd pbc-$ver && CFLAGS='@CFLAGS' CPPFLAGS='@CFLAGS' LDFLAGS='@LFLAGS' ./configure && \$(MAKE) \$(PASTHRU)
";

}

sub unpack_libpbc {
    my $ver = shift;
    return 1 if -d "pbc-$ver" or $ENV{SKIP_DOWNLOAD};

    warn "unpacking libpbc from pbc-$ver.tar.gz...\n";

    my $worked = 0;
    eval {
        eval "require Archive::Tar;"; die $@ if $@;
        my $tar = Archive::Tar->new;

        $tar->read("pbc-$ver.tar.gz", 1);
        $tar->extract;

        $worked = 1 if -x "pbc-$ver/configure";
    };
    warn "Archive::Tar unpack problem: $@\n" if $@;

    unless( $worked ) {
        if( system("gzip -dc pbc-$ver.tar.gz | tar -xvf -") == 0 ) {
            $worked = 1 if -x "pbc-$ver/configure";
        }
    }

    warn "failed to unpack pbc\n" unless $worked;
    $worked;
}

sub grab_libpbc {
    my $ver = shift;
    return 1 if -f "pbc-$ver.tar.gz" or $ENV{SKIP_DOWNLOAD};

    warn "downloading libpbc from http://voltar.org/pbcfiles/pbc-$ver.tar.gz...\n";

    my $worked = 0;
    eval {
        eval "require LWP::UserAgent"; die $@ if $@;
        my $ua = LWP::UserAgent->new;
           $ua->agent("crypt-pbc-fetcher/1.0");

        my $req = HTTP::Request->new(GET => "http://voltar.org/pbcfiles/pbc-$ver.tar.gz");
        my $res = $ua->request($req, "pbc-$ver.tar.gz");

        $worked = 1 if $res->is_success;
    };
    warn "LWP fetch problem: $@\n" if $@;

    unless( $worked ) {
        if( 0 != system(wget => '-O', "pbc-$ver.tar.gz", "http://voltar.org/pbcfiles/pbc-$ver.tar.gz") ) {
            warn "couldn't fetch with wget...\n";
            if( 0 != system(curl => '-o', "pbc-$ver.tar.gz", "http://voltar.org/pbcfiles/pbc-$ver.tar.gz") ) {
                warn "couldn't fetch with curl ...\n";
            }
        }

        $worked = 1 if -f "pbc-$ver.tar.gz";
    }

    warn "failed to download pbc\n" unless $worked;
    $worked;
}
