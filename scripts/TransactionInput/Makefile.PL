use ExtUtils::MakeMaker;
my %options = %{
{
  'TYPEMAPS' => [],
  'NAME' => 'CBitcoin::TransactionInput',
  'INC' => '',
  'VERSION' => '0.01'
}
};
WriteMakefile(%options);

# Remove the Makefile dependency. Causes problems on a few systems.
sub MY::makefile { '' }
