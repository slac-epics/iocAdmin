#!/usr/bin/perl

BEGIN
{
    push(@INC, 'scripts' );
    push(@INC, '/arch/scripts' );
}

use English;
use strict;
use vars qw($opt_d $opt_h $opt_c);
use Getopt::Std;
use Sys::Hostname;
use archiveconfig;

# Globals, Defaults
my ($config_name) = "archiveconfig.xml";
my ($localhost) = hostname();

# Configuration info filled by parse_config_file
my ($config);

sub usage()
{
    print("USAGE: start_daemons.pl [options]\n");
    print("\n");
    print("Options:\n");
    print(" -h          : help\n");
    print(" -c <config> : Use given config file instead of $config_name.\n");
    print(" -d          : debug\n");
}

# The main code ==============================================

# Parse command-line options
if (!getopts("dhc:") ||  $opt_h)
{
    usage();
    exit(0);
}
$config_name = $opt_c if (length($opt_c) > 0);

$config = parse_config_file($config_name, $opt_d);
my ($root) = $config->{root};
my ($d_dir, $e_dir, $line);
foreach $d_dir ( sort keys %{ $config->{daemon} } )
{
    next unless is_localhost($config->{daemon}{$d_dir}{'run'});
    foreach $e_dir ( sort keys %{ $config->{daemon}{$d_dir}{engine} } )
    {
        next unless is_localhost($config->{daemon}{$d_dir}{engine}{$e_dir}{run});
        my ($port) = $config->{daemon}{$d_dir}{engine}{$e_dir}{port};
        printf("%40s\t%d\t",
               $config->{daemon}{$d_dir}{engine}{$e_dir}{desc}, $port);
        my ($version) = "unknown";
        foreach $line ( read_URL("localhost", $port, "") )
        {
            if ($line =~ m'Version</TH><TD ALIGN=CENTER>(.+)</TD>')
            {
                $version = $1;
                last;
            }
        }
        print "$version\n";
    }
}

