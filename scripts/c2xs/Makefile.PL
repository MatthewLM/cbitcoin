use ExtUtils::MakeMaker;
my %options = %{
{
  'TYPEMAPS' => [],
  'NAME' => 'CBitcoin::CBHD',
  'INC' => '/home/joeldejesus/Workspace/cbitcoin/library/include/',
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
'-lcbitcoin-rand.2.0'],
  'VERSION' => '0.01'
}
};
WriteMakefile(%options);

# Remove the Makefile dependency. Causes problems on a few systems.
sub MY::makefile { '' }
