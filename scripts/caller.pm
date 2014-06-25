 use CBitcoin::CBHD;

my $masterkey = new CBitcoin::CBHD;
$masterkey->generate();
my $wif = $masterkey->WIF();

print "Wif:$wif\n";
