 use InlineX::C2XS qw(c2xs);


my $module_name = 'CBitcoin::Transaction';
my $package_name = 'CBitcoin::Transaction';

my $config_opts = {'WRITE_PM' => 1,
                     'WRITE_MAKEFILE_PL' => 1,
                     'VERSION' => 0.01,
                    };


c2xs($module_name, $package_name,'./',$config_opts);


