#!/usr/bin/perl -w

$#ARGV==2 or die "usage: doit.pl range_max num_keys num_lookups\n";

($range_max, $num_keys, $num_lookups) = @ARGV;



$cmd = "./zipf_gen.pl $range_max $num_keys $num_lookups | ./run-splay $num_keys $num_lookups";

system $cmd;
