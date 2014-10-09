use ExtUtils::MakeMaker;
my %options = %{
{
  'TYPEMAPS' => [],
  'NAME' => 'CBitcoin::TransactionInput',
  'INC' => '../../library/include',
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
  ,'MYEXTLIB' => '/usr/local/lib/libcbitcoin.2.0.so /usr/local/lib/libcbitcoin-file-no-ec.2.0.so /usr/local/lib/libcbitcoin-rand.2.0.so /usr/local/lib/libcbitcoin-threads.2.0.so /usr/local/lib/libcbitcoin-network.2.0.so /usr/local/lib/libcbitcoin-file-ec.2.0.so /usr/local/lib/libcbitcoin-storage.2.0.so /usr/local/lib/libcbitcoin-crypto.2.0.so /usr/local/lib/libcbitcoin-logging.2.0.so '
  ,'VERSION' => '0.01'
  ,'CC' => 'c99'
}
};
WriteMakefile(%options);

# Remove the Makefile dependency. Causes problems on a few systems.
sub MY::makefile { '' }
