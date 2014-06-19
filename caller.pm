
 use CBitcoin::CBHD;



my $masterkey = new CBitcoin::CBHD;
$masterkey->generate();
my $masterkeywif = $masterkey->WIF();
print "hi($masterkeywif)\n";

