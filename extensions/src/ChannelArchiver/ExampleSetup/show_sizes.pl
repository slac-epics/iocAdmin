#!/usr/bin/perl
# show_sizes.pl
#
# kasemirk@ornl.gov

BEGIN
{
    push(@INC, 'scripts' );
    push(@INC, '/arch/scripts' );
}

use English;
use strict;
use vars qw($opt_d $opt_h $opt_c);
use Getopt::Std;
use Data::Dumper;
use Sys::Hostname;
use archiveconfig;

# Globals, Defaults
my ($config_name) = "archiveconfig.xml";

# Configuration info filled by parse_config_file
my ($config);

sub usage()
{
    print("USAGE: show_sizes [options]\n");
    print("\n");
    print("Options:\n");
    print(" -h          : help\n");
    print(" -c <config> : Use given config file instead of $config_name\n");
    print(" -d          : debug\n");
}

sub format_bytes($)
{
    my ($bytes) = @ARG;
    my ($giga, $mega, $kilo);

    $giga = int($bytes/1024.0/1024/1024);
    $bytes -= $giga*1024*1024*1024;

    $mega = int($bytes/1024.0/1024);
    $bytes -= $mega*1024*1024;

    $kilo = int($bytes/1024.0);
    $bytes -= $kilo*1024;

    return sprintf("%4lu GB, %4lu MB, %4lu kB", $giga, $mega, $kilo);
}

sub show_sizes()
{
    my ($d_dir, $e_dir, $dir, $info, %bytes);
    my ($root) = $config->{root};
    my ($total) = 0;
    print("Data sizes for the engine directories:\n");
    foreach $d_dir ( keys %{ $config->{daemon} } )
    {
	foreach $e_dir ( keys %{ $config->{daemon}{$d_dir}{engine} } )
	{
            $dir = "$root/$d_dir/$e_dir";
	    print("du -bs $dir ...\n") if ($opt_d);
	    open(DU, "du -bs $dir |") or die "Cannot run 'du'\n";
            $info = <DU>;
	    close DU;
            chomp($info);
            if ($info =~ m"([0-9]+)\s+(\S+)")
            {
                $bytes{$dir} = $1;
                $total += $1;
                printf("%-30s%s\n", $dir, format_bytes($bytes{$dir}));
            }
            else
            {
	        print "$info";
            }
	}
    }
    printf("%-30s%s\n", "Total", format_bytes($total));
}

# The main code ==============================================

# Parse command-line options
if (!getopts("dhc:") ||  $opt_h)
{
    usage();
    exit(0);
}
$config_name = $opt_c  if (length($opt_c) > 0);
$config = parse_config_file($config_name, $opt_d);
show_sizes();

