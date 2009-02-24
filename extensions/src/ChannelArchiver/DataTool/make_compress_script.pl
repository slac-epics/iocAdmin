#!/usr/bin/perl

use English;
use strict;
use File::Find;
use Time::Local;
use vars qw($opt_h $opt_d $opt_n);
use Getopt::Std;

# Globals -------------------------------------------------------------
# Path names to all the sub-archives,
# used by find()/locate_sub_archive_paths
my (@sub_archive_paths);

# $time = 2004/07/20 00:00:00
# $archives{$time}->{path} = "path/2004/07_20";
my (%archives);

# Name of new index
my ($new_index);

sub usage()
{
    print("USAGE: make_compress_script.pl [options] {path}\n");
    print("\n");
    print("Options:\n");
    print("\t-n <path/index>: full path to new index\n");
    print("\t-h             : help\n");
    print("\t-d             : debug mode\n");
    print("\n");
    print(" {path}          : one or more paths to index files\n");
    print("                   (path only, e.g. \"2004/07_*\")\n");
    print("\n");
    print("This script creates an example shell script for\n");
    print("invoking the ArchiveDataTool in order to combine\n");
    print("daily or weekly sub-archives into bigger ones.\n");
    print("It is usually advisable to check the resulting\n");
    print("scipt before running it.\n");
    exit(-1);
}

sub locate_sub_archive_paths()
{
    my ($index) = $_;
    return unless ($index eq "index");
    print("Found index in '$File::Find::dir'\n") if ($opt_d);
    push @sub_archive_paths, $File::Find::dir;
}

sub analyze_sub_archives()
{
    my ($subarchive, $time, $line);
    foreach $subarchive ( sort @sub_archive_paths )
    {
	print "Analyzing $subarchive\n" if ($opt_d);
	if ($subarchive =~ '(.*/)?([0-9]{4})/([0-9]{2})_([0-9]{2})\Z')
	{   # Format path/YYYY/MM_DD
	    $time = timelocal(0, 0, 0, $4, $3-1, $2-1900);
	}
	elsif ($subarchive =~ '(.*/)?([0-9]{4})/([0-9]{2})_([0-9]{2})_([0-9]{2})\Z')
	{   # Format path/YYYY/MM_DD_HH
	    $time = timelocal(0, 0, $5, $4, $3-1, $2-1900);
	}
	else
	{
	    die "Cannot decode the format of the sub-archive in '".
		$subarchive . "'\n";
	}
	die "Is $subarchive a duplicate?" if (exists($archives{$time}));
	# Get Info via ArchiveDataTool
	open(DT, "ArchiveDataTool -i -v 0 $subarchive/index|")
	    or die "Cannot run ArchiveDataTool\n";
	$line=<DT>;
	close(DT);
	# 269 channels, 06/14/2004 09:27:34.455077000 - 07/09/2004 07:00:25.754153000
	if ($line =~ m'([0-9]+) channels, ([0-9]{2}/[0-9]{2}/[0-9]{4}) ([0-9:.]{18}) - ([0-9]{2}/[0-9]{2}/[0-9]{4}) ([0-9:.]{18})')
	{
	    $archives{$time}->{channels} = $1;
	    $archives{$time}->{start_date} = $2;
	    $archives{$time}->{start_time} = $3;
	    $archives{$time}->{end_date} = $4;
	    $archives{$time}->{end_time} = $5;
	}
	else
	{
	    die "cannot parse:\n$line";
	}
	$archives{$time}->{path} = $subarchive;
    }
}

sub time2txt($)
{
    my ($time) = @ARG;
    my ($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst)
	= localtime($time);
    return sprintf("%02d/%02d/%04d %02d:%02d:%02d",
		   $mon+1, $mday, 1900+$year, $hour, $min, $sec);
}

sub create_script()
{
    my (@nominal_sub_times) = sort { $a <=> $b } keys %archives;
    my ($N) = $#nominal_sub_times + 1;
    my ($i, $t0, $t1, $tx, $start, $end, $diff, $prev_diff);
    $prev_diff = 0;

    die "No archives found\n" if ($N < 1);

    if (length($new_index) <= 0)
    {
	my ($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst)
	    = localtime($nominal_sub_times[0]);
	$new_index = sprintf("%04d/%02d/index", 1900+$year, $mon+1);
    }
    print "# Combine the following $N sub-archives into $new_index:\n";
    for ($i=0; $i<$N; ++$i)
    {
	$t0 = $nominal_sub_times[$i];
	print "# $archives{$t0}->{path}: ";
	printf "%5d channels, ", $archives{$t0}->{channels};
	print "$archives{$t0}->{start_date} $archives{$t0}->{start_time}";
	print " - ";
	print "$archives{$t0}->{end_date} $archives{$t0}->{end_time}\n";
    }
    print "\n";
    print "# Check that $new_index can be created, i.e. the path exists!\n";
    print "mkdir -p `dirname $new_index`\n";
    print "# The following starts the copy process at the end date of the previous sub-archive.\n";
    print "# You need to check these for consistency, e.g.:\n";
    print "# Does it match your planned switch-over between sub-archives?\n";
    print "# Otherwise a broken sub-archive that ran amok with nonsense time stamps far in the future\n";
    print "# will prevent valid data from the next sub-archive from being copied\n";
    print "# (since we can't go back in time).\n";
    $prev_diff = undef;
    for ($i=0; $i<$N; ++$i)
    {
	# Check this and next sub-archive: Difference, nominal start/stop
	$t0 = $nominal_sub_times[$i];
	if ($i < $N-1)
	{
	    $t1 = $nominal_sub_times[$i+1];
	    $diff = $t1 - $t0;
	}
	else
	{
	    $diff = undef;
	}
	# Use previous sub's actual end time for start of copy
	if ($i > 0)
	{
	    $tx = $nominal_sub_times[$i-1];
	    $start = "$archives{$tx}->{end_date} $archives{$tx}->{end_time}";
	}
	else
	{
	    $start = undef;
	}
	$end = undef;

	print "nice ArchiveDataTool -v 1 $archives{$t0}->{path}/index ";
	print "-c $new_index";
	print " -s '$start'" if (defined($start));
	print " -e '$end'"   if (defined($end));
	print "\n";
	if (defined($prev_diff) and defined($diff) and $diff != $prev_diff)
	{
	    if ($diff >= 24*60*60)
	    {
		printf("# NOTE: Interval changed from %g to %g days!\n",
		       $prev_diff/(24*60*60), $diff/(24*60*60));
	    }
	    else
	    {
		printf("# NOTE: Interval changed from %g to %g hours!\n",
		       $prev_diff/(60*60), $diff/(60*60));
	    }
	}
	$prev_diff = $diff;
    }
}

# ----------------------------------------------------------------
# Main
# ----------------------------------------------------------------

if (!getopts('hdn:')  ||  $#ARGV < 0  ||  $opt_h)
{
    usage();
}

find(\&locate_sub_archive_paths, @ARGV);
analyze_sub_archives();
$new_index = $opt_n if (length($opt_n));
create_script();
