#!/usr/bin/perl

BEGIN
{
    push(@INC, 'scripts' );
    push(@INC, '/arch/scripts' );
}

use English;
use strict;
use vars qw($opt_h $opt_c $opt_s);
use Cwd;
use File::Path;
use Getopt::Std;
use Data::Dumper;
use Sys::Hostname;
use archiveconfig;

# Globals, Defaults
my ($config_name) = "archiveconfig.xml";
my ($hostname)    = hostname();
my ($path) = cwd();
my ($index_dtd, $daemon_dtd, $engine_dtd);

sub usage()
{
    print("USAGE: check_config [options]\n");
    print("\n");
    print("Reads and dumps a configuration for testing.\n");
    print("\n");
    print("Options:\n");
    print(" -h          : help\n");
    print(" -c <config> : Use given config file instead of $config_name\n");
}

# Configuration info filled by parse_config_file
my ($config);

# The main code ==============================================

# Parse command-line options
if (!getopts("hc:") ||  $opt_h)
{
    usage();
    exit(0);
}
$config_name = $opt_c if (length($opt_c) > 0);
# Parse with debugging enabled causes dump of info
$config = parse_config_file($config_name, 1);
    
