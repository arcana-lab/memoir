#!/usr/bin/perl -w

use Math::Random::Zipf;

$#ARGV==2 or die "usage: zipf_gen.pl range_max num_keys num_lookups\n";

($range_max, $num_keys, $num_lookups) = @ARGV;

$z = Math::Random::Zipf->new($range_max,1);


#generate key set with no collisions
for ($i=0;$i<$num_keys;$i++) {
    do {
	$key = $z->rand();
    } while (defined($keyset{$key}));
    $keyset{$key}=1;
    print "insert ",$key,"\n";  
}

#generate lookup stream, including lookups that
#are for non-existent keys
for ($i=0;$i<$num_lookups;$i++) {
    print "lookup ",$z->rand(),"\n";
}
    
