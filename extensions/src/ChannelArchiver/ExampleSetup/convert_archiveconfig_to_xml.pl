#!/usr/bin/perl
# make_archive_infofile.pl

BEGIN
{
    push(@INC, 'scripts' );
    push(@INC, '/arch/scripts' );
}

use English;
use strict;
use vars qw($opt_h $opt_c $opt_d);
use Cwd;
use Getopt::Std;
use Data::Dumper;
use Sys::Hostname;
use archiveconfig;

my ($localhost) = hostname();

# Globals, Defaults
my ($config_name) = "archiveconfig.csv";

# Configuration info filled by parse_config_file
my ($config);

sub usage()
{
    print("USAGE: convert_archiveconfig_to_xml.pl [options]\n");
    print("\n");
    print("Options:\n");
    print(" -h          : help\n");
    print(" -d          : debug\n");
    print(" -c <config> : Use given config file instead of $config_name\n");
}

# Input: Reference to @daemons
sub dump_config_as_xml($)
{
    my ($config) = @ARG;
    my ($d_dir, $e_dir);
    print("<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\n");
    print("<!DOCTYPE archiveconfig SYSTEM \"archiveconfig.dtd\">\n");
    print("\n");
    print("<!-- Archive Configuration\n");
    print("\n");
    print("  -->\n");
    print("<archive_config>\n");
    my ($root) = cwd();
    print("<root>$root</root>\n");
    foreach $d_dir ( keys %{ $config->{daemon} } )
    {
        printf("\n    <daemon directory='%s'>\n", $d_dir);
        printf("        <run>$localhost</run>\n");
        printf("        <desc>%s</desc>\n",
               $config->{daemon}{$d_dir}{desc});
        printf("        <port>%s</port>\n",
               $config->{daemon}{$d_dir}{port});
        foreach $e_dir ( keys %{ $config->{daemon}{$d_dir}{engine} } )
        {
            printf("        <engine directory='%s'>\n", $e_dir);
            printf("            <run>$localhost</run>\n");
            printf("            <desc>%s</desc>\n",
                   $config->{daemon}{$d_dir}{engine}{$e_dir}{desc});
            printf("            <port>%s</port>\n",
                   $config->{daemon}{$d_dir}{engine}{$e_dir}{port});
            printf("            <restart type='%s'>%s</restart>\n",
                   $config->{daemon}{$d_dir}{engine}{$e_dir}{restart}{type},
                   $config->{daemon}{$d_dir}{engine}{$e_dir}{restart}{content});
            printf("        </engine>\n");
        }
        printf("    </daemon>\n");
    }
    print("\n</archive_config>\n");
}

# The main code ==============================================

# Parse command-line options
if (!getopts("hdc:") ||  $opt_h)
{
    usage();
    exit(0);
}
$config_name = $opt_c  if (length($opt_c) > 0);

$config = parse_config_file($config_name, $opt_d);
dump_config_as_xml($config);
