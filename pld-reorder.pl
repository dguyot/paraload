#!/usr/bin/perl -w

use strict;
use Getopt::Long;

my $log_file;
my $res_file;
my $out_file;
my $quiet_flag;
my $only_check;
my $return = 0;
my $help_flag;

my %index_e;
#my %input_offset_b;
#my %input_offset_e;
my %output_offset_b;
my %output_offset_e;
my %return_code;

my $line;
my @split_line;
my $begin_index;
my $end_index;
my $last_index = 0;
my $i;

my $chunk;
my $chunk_size;

GetOptions("log=s" => \$log_file,"res=s" => \$res_file,"out=s" => \$out_file,"only_check" => \$only_check,"quiet" => \$quiet_flag,"return=i" => \$return,"help" => \$help_flag);

if ($help_flag)
{
	print "$0 \n\t--log [log file of paraload] \n\t--res [result file of paraload] \n\t--out [output sorted] \n\t{--return [return code you want to select default: 0]}\n\t{--quiet (ignore warnings)}\n\t{--only_check (do not write ontput)}\n\n";
	print "lines between {} are optional\n";
	exit(0);
}

if ($only_check && $quiet_flag)
{
	print "The check flag and the quiet flag are imcompatibles\n"
}


open(LOG_FILE,$log_file) or die("$log_file does not exists");

foreach $line (<LOG_FILE>)
{
	chomp($line);
	@split_line = split('\t',$line);
	$begin_index = $split_line[0];
	$end_index = $split_line[1];
	$index_e{$begin_index} = $end_index;
#	$input_offset_b{$begin_index} = $split_line[2];
#	$input_offset_e{$begin_index} = $split_line[3];
	$output_offset_b{$begin_index} = $split_line[4];
	$output_offset_e{$begin_index} = $split_line[5];
	$return_code{$begin_index} = $split_line[6];
	$last_index = $end_index - 1 if ($last_index < $end_index);
}

close(LOG_FILE);

if (!$quiet_flag)
{
	$end_index = 0;
	for ($i = 0; $i <= $last_index; $i++)
	{
		if (exists($index_e{$i}))
		{
			if ($i != $end_index)
			{
				print "Missing segment of index: [$i\t$index_e{$i}]\n";
			}
			if ($return_code{$i} != $return)
			{
				print "Bad return of index [$i\t$index_e{$i}] : $return_code{$i}\n";
			}
			$end_index = $index_e{$i};
		}
	}
}

if (!$only_check)
{
	open(RES,$res_file) or die("$res_file does not exists");
	open(OUT,">",$out_file) or die("$out_file does not exist");
	
	for ($i = 0; $i <= $last_index; $i++)
	{
		if (exists($index_e{$i}) && ($return_code{$i} == $return))
		{
			$chunk_size = $output_offset_e{$i}-$output_offset_b{$i};
			seek(RES,$output_offset_b{$i},0);
			read(RES,$chunk,$chunk_size);
			print OUT $chunk;
		}
	}
		
	close(RES);
	close(OUT);
}

exit(0);
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
