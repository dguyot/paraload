#!/usr/bin/perl -w

#~ Copyright or © or Copr. Dominique GUYOT 2016
#~ 
#~ dominique.guyot@univ-lyon1.fr
#~ 
#~ This software is a computer program whose purpose is to make load balancing
#~ for very large data parallel computing. 
#~ 
#~ This software is governed by the CeCILL license under French law and
#~ abiding by the rules of distribution of free software.  You can  use, 
#~ modify and/ or redistribute the software under the terms of the CeCILL
#~ license as circulated by CEA, CNRS and INRIA at the following URL
#~ "http://www.cecill.info". 
#~ 
#~ As a counterpart to the access to the source code and  rights to copy,
#~ modify and redistribute granted by the license, users are provided only
#~ with a limited warranty  and the software's author,  the holder of the
#~ economic rights,  and the successive licensors  have only  limited
#~ liability. 
#~ 
#~ In this respect, the user's attention is drawn to the risks associated
#~ with loading,  using,  modifying and/or developing or reproducing the
#~ software by the user in light of its specific status of free software,
#~ that may mean  that it is complicated to manipulate,  and  that  also
#~ therefore means  that it is reserved for developers  and  experienced
#~ professionals having in-depth computer knowledge. Users are therefore
#~ encouraged to load and test the software's suitability as regards their
#~ requirements in conditions enabling the security of their systems and/or 
#~ data to be ensured and,  more generally, to use and operate it in the 
#~ same conditions as regards security. 
#~ 
#~ The fact that you are presently reading this means that you have had
#~ knowledge of the CeCILL license and that you accept its terms.




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
