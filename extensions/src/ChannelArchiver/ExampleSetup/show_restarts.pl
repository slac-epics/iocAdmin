#!/usr/bin/perl
# make_archive_infofile.pl

BEGIN
{
    push(@INC, 'scripts' );
    push(@INC, '/arch/scripts' );
}

use English;
use strict;
use vars qw($opt_h $opt_c);
use Getopt::Std;
use Data::Dumper;
use Sys::Hostname;
use archiveconfig;

# Globals, Defaults
my ($config_name) = "archiveconfig.xml";
my ($output_name) = "-";
my ($localhost) = hostname();

sub usage()
{
    print("USAGE: show_restarts [options]\n");
    print("\n");
    print("Options:\n");
    print(" -h          : help\n");
    print(" -c <config> : Use given config file instead of $config_name\n");
}

sub show_restarts($)
{
    my ($config) = @ARG;
    my ($d_dir, $e_dir, $ok, $out, $disconnected);
    my ($type, $time, %restarts);

    foreach $d_dir ( sort keys %{ $config->{daemon} } )
    {
        next unless is_localhost($config->{daemon}{$d_dir}{'run'});
        # print("$d_dir:\n");
	foreach $e_dir ( sort keys %{ $config->{daemon}{$d_dir}{engine} } )
	{
            next unless
                 is_localhost($config->{daemon}{$d_dir}{engine}{$e_dir}{'run'});
            $type = $config->{daemon}{$d_dir}{engine}{$e_dir}{restart}{type};
            $time = $config->{daemon}{$d_dir}{engine}{$e_dir}{restart}{content};
            # print("    $e_dir  $type $time\n");
            # For each daemon, keep a sorted list of restart times:
            push @{ $restarts{$d_dir} }, "$time ($e_dir, $type)";
            @{ $restarts{$d_dir} } = sort @{ $restarts{$d_dir} };
	}
    }
    #print Dumper(\%restarts);
    # Print, sorted by the restart time of the first (earliest) engine restart
    # under each daemon:
    foreach $d_dir ( sort { $restarts{$a}[0] cmp $restarts{$b}[0] } keys %restarts )
    {
        print("$d_dir:\n");
	foreach $e_dir ( @{ $restarts{$d_dir} } )
        {
            print("    $e_dir\n");
        }
    }
}

# The main code ==============================================

# Parse command-line options
if (!getopts("hc:") ||  $opt_h)
{
    usage();
    exit(0);
}
$config_name = $opt_c  if (length($opt_c) > 0);

my ($config) = parse_config_file($config_name, 0);
show_restarts($config);
