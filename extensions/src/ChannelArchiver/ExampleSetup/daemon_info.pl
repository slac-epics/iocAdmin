#!/usr/bin/perl

BEGIN
{
    push(@INC, 'scripts' );
    push(@INC, '/arch/scripts' );
}

use English;
use strict;
use vars qw($opt_d $opt_h $opt_c $opt_p);
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
    print("USAGE: daemon_info.pl [options]\n");
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

my ($d_dir);

foreach $d_dir ( keys %{ $config->{daemon} } )
{
     printf("Daemon '%s' on port %d:\n",
            $d_dir, $config->{daemon}{$d_dir}{port});
     system("lynx -dump http://$localhost:$config->{daemon}{$d_dir}{port}/info");
}

