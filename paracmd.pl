#!/usr/bin/perl -w

use strict;

my $cmd;
my $rt = 0;

#signal handler

$SIG{HUP} = \&sighandler;
$SIG{TERM} =\&sighandler;

sub sighandler
{
	exit($rt);
}

#print $ARGV[0];
open(LIST,$ARGV[0]);
while ($cmd=<LIST>)
{
	$rt = system($cmd);
	if ($rt != 0)
        {
                exit($rt);
        }
}
close(LIST);

exit(0);
