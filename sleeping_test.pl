#!/usr/bin/perl -w 

use strict;
use warnings;

my $sleep;

open(FIN,$ARGV[0]);

foreach $sleep (<FIN>)
{
	chomp($sleep);
	print "Sleeping $sleep seconds\n";
	sleep($sleep);
	print "Wake up after $sleep seconds\n";
}

close(FIN);

exit(0);
