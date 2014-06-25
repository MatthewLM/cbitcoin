use ExtUtils::MakeMaker;
my %options = %{
{
  'TYPEMAPS' => [],
  'NAME' => 'CBitcoin::CBHD',
  'INC' => '/home/joeldejesus/Workspace/cbitcoin/library/include',
  'LIBS' => ['-lcbitcoin.2.0',
'-lcbitcoin-network.2.0',
'-lcbitcoin-storage.2.0',
'-lcbitcoin-threads.2.0',
'-lpthread',
'-lcbitcoin-logging.2.0',
'-lcbitcoin-crypto.2.0',
'-lcrypto',
'-lcbitcoin.2.0',
'-lcbitcoin-file-ec.2.0',
'-lcbitcoin-rand.2.0']
  ,'MYEXTLIB' => '/usr/local/lib/libcbitcoin.2.0.so /usr/local/lib/libcbitcoin-file-no-ec.2.0.so /usr/local/lib/libcbitcoin-rand.2.0.so /usr/local/lib/perl/5.14.2/auto/File/Map/Map.so /usr/local/lib/perl/5.14.2/auto/Crypt/PBC/PBC.so  /usr/local/lib/perl/5.14.2/auto/Math/BigInt/GMP/GMP.so /usr/local/lib/perl/5.14.2/auto/BerkeleyDB/BerkeleyDB.so /usr/local/lib/perl/5.14.2/auto/PerlIO/Layers/Layers.so /usr/local/lib/libcbitcoin-threads.2.0.so /usr/local/lib/libcbitcoin-network.2.0.so /usr/local/lib/libcbitcoin-file-ec.2.0.so /usr/local/lib/libcbitcoin-storage.2.0.so /usr/local/lib/libcbitcoin-crypto.2.0.so /usr/local/lib/libcbitcoin-logging.2.0.so '
  ,'VERSION' => '0.02'
}
};
WriteMakefile(%options);

# Remove the Makefile dependency. Causes problems on a few systems.
sub MY::makefile { '' }
