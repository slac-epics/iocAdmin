#!/usr/bin/perl
#
# See usage()
# and scp comments in update_server.pl

BEGIN
{
    push(@INC, 'scripts' );
    push(@INC, '/arch/scripts' );
}

use English;
use strict;
use vars qw($opt_d $opt_h $opt_c $opt_u);
use Getopt::Std;
use Sys::Hostname;
use File::Path;
use archiveconfig;

# Globals, Defaults
my ($config_name) = "archiveconfig.xml";
my ($localhost) = hostname();

# Configuration info filled by parse_config_file
my ($config);

sub usage()
{
    print("USAGE: sent_mailbox.pl [options] <target-host>\n");
    print("\n");
    print(" Reads the mailbox location from $config_name\n");
    print(" and uses scp to copy all files to given host.\n");
    print(" Use this in case you don't have a shared NFS mailbox.\n");
    print("\n");
    print("\n");
    print("\n");
    print("Options:\n");
    print(" -h          : help\n");
    print(" -c <config> : Use given config file instead of $config_name.\n");
    print(" -u <user>   : Use given user name.\n");
    print(" -d          : debug\n");
}

# The main code ==============================================

# Parse command-line options
if (!getopts("dhc:u:")  ||  $#ARGV != 0  ||  $opt_h)
{
    usage();
    exit(0);
}
$config_name = $opt_c if (length($opt_c) > 0);
my ($target) = $ARGV[0];
if (length($opt_u) > 0)
{
    $target = "$opt_u\@$target";
}

$config = parse_config_file($config_name, $opt_d);
my ($root) = $config->{root};
die "No mailbox directory configured in $config_name\n"
    unless (exists($config->{mailbox}));

my ($mb) = $config->{mailbox};
chdir($mb);
mkpath($target, 1);
my ($file);
foreach $file ( <*.txt> )
{
     # TODO: Check if the file should really go to that target,
     # so that eventually we can support multiple targets.
     my ($cmd) = "scp $config->{mailbox}/$file $target:$config->{mailbox}/$file && mv $file $target";
     printf("$cmd\n");
     system($cmd);
}
rmtree($target, 1);

