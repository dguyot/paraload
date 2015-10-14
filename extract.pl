#!/usr/bin/perl -w

use strict;
use Getopt::Long;

my $begin;
my $end;
my $size;
my $file;
my $chunk;
my $help;

GetOptions("b=i" =>\$begin, "e=i" => \$end, "f=s" =>\$file, "help" =>\$help);

if ($help)
{
	print STDOUT "Extract a chunk of a file by delimiters (in bytes)\n\n";
	print STDOUT "$0 -b [beginning of the chunk in the file (in byte)] ";
	print STDOUT "-e [end of the chunk in the file (in byte)] ";
	print STDOUT "-f [the file containing the chunk]\n\n";
	exit(0);
}

$size = $end - $begin;
if ($size >= 0)
{
	open(FIC,$file) or die("$file does not exists!\n");
	seek(FIC,$begin,0);
	read(FIC,$chunk,$size);
	print STDOUT $chunk;
	close(FIC);
}
else
{
	print STDERR "end must be greater than begin.\n";
	exit(1);
}
exit(0);
