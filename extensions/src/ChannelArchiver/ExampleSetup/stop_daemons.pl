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
    print("USAGE: stop_daemons.pl [options]\n");
    print("\n");
    print("Options:\n");
    print(" -h          : help\n");
    print(" -c <config> : Use given config file instead of $config_name.\n");
    print(" -p          : postal (kill engines as well)\n");
    print(" -d          : debug\n");
}

# The main code ==============================================

# Parse command-line options
if (!getopts("dhpc:") ||  $opt_h)
{
    usage();
    exit(0);
}
$config_name = $opt_c if (length($opt_c) > 0);

$config = parse_config_file($config_name, $opt_d);

my ($stop, $d_dir, $quit, $line, @html);
$stop = "stop";
$stop = "postal" if ($opt_p);

foreach $d_dir ( keys %{ $config->{daemon} } )
{
     printf("Stopping Daemon '%s' on port %d: ",
            $d_dir, $config->{daemon}{$d_dir}{port});
     $quit = 0;
     @html = read_URL($localhost, $config->{daemon}{$d_dir}{port}, $stop);
     foreach $line ( @html )
     {
         if ($line =~ "Quitting")
         {
             $quit = 1;
             last;
         }
     }
     if ($quit)
     {
         print("Quit.\n");
     }
     else
     {
         print ("Didn't quit. Response:\n");
         foreach $line ( @html )
         {
             print("'$line'\n");
         }
     }
}

if ($opt_p)
{
    print("Check\n  ls -l */*/archive_active.lck\nto see which engines are still 'up'\n");
}
