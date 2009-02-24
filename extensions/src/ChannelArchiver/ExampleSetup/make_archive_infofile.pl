#!/usr/bin/perl
# make_archive_infofile.pl

BEGIN
{
    push(@INC, 'scripts' );
    push(@INC, '/arch/scripts' );
}

use English;
use strict;
use vars qw($opt_d $opt_h $opt_c $opt_o $opt_e);
use Getopt::Std;
use Data::Dumper;
use Sys::Hostname;
use archiveconfig;

# Globals, Defaults
my ($config_name) = "archiveconfig.xml";
my ($output_name) = "-";
my ($localhost) = hostname();

# Configuration info filled by parse_config_file
my ($config);

sub usage()
{
    print("USAGE: make_archive_infofile [options]\n");
    print("\n");
    print("Options:\n");
    print(" -h          : help\n");
    print(" -c <config> : Use given config file instead of $config_name\n");
    print(" -o <html>   : Generate given output file instead of $output_name\n");
    print(" -d          : debug\n");
}

sub write_info($)
{
    my ($filename) = @ARG;
    my ($d_dir, $e_dir, $ok, $out, $disconnected);
    my ($issues, $status);

    $status = "Archive Status as of " . time_as_text(time) . "\n"
         . "--------------------------------------------------\n";
    foreach $d_dir ( sort keys %{ $config->{daemon} } )
    {
        next unless is_localhost($config->{daemon}{$d_dir}{'run'});
	$status = $status .
                  sprintf("Daemon %-25s: %s\n",
                          "'" . $d_dir ."'", $config->{daemon}{$d_dir}{status});
	$issues = $issues .
                  sprintf("Daemon %-25s: %s\n",
                          "'" . $d_dir ."'", $config->{daemon}{$d_dir}{status})
            if ($config->{daemon}{$d_dir}{status} ne "running");
	foreach $e_dir ( sort keys %{ $config->{daemon}{$d_dir}{engine} } )
	{
            next unless
                 is_localhost($config->{daemon}{$d_dir}{engine}{$e_dir}{'run'});
            if ($config->{daemon}{$d_dir}{engine}{$e_dir}{status} eq "running")
            {
                $disconnected = $config->{daemon}{$d_dir}{engine}{$e_dir}{channels}
                     - $config->{daemon}{$d_dir}{engine}{$e_dir}{connected};
                if ($disconnected == 0)
                {
                    $status = $status .
                        sprintf("Engine %-25s: %5d channels.\n",
                                "'$e_dir'",
                                $config->{daemon}{$d_dir}{engine}{$e_dir}{channels});
                }
                else
                {
                    $status = $status .
                        sprintf("Engine %-25s: %5d channels,  %5d disconnected.\n",
                                "'$e_dir'",
                                $config->{daemon}{$d_dir}{engine}{$e_dir}{channels},
                                $disconnected);
                    $issues = $issues .
                        sprintf("Engine %-25s: %5d channels,  %5d disconnected.\n",
                                "'$d_dir/$e_dir'",
                                $config->{daemon}{$d_dir}{engine}{$e_dir}{channels},
                                $disconnected);
                }
            }
            else
            {
                $status = $status .
                        sprintf("Engine %-25s: %s\n",
                                "'$e_dir'",
                                $config->{daemon}{$d_dir}{engine}{$e_dir}{status});
                if ($config->{daemon}{$d_dir}{engine}{$e_dir}{status} ne "disabled")
                {
                    $issues = $issues .
                        sprintf("Engine %-25s: %s\n",
                                "'$d_dir/$e_dir'",
                                $config->{daemon}{$d_dir}{engine}{$e_dir}{status});
                }
            }  
	}
	$status = $status . "\n";
    }

    open($out, ">>$filename") or die "Cannot open '$filename'\n";
    if (length($issues) > 0)
    {
        print $out "Possible Issues\n";
        print $out "--------------------------------------------------\n";
        print $out "$issues\n\nComplete ";
    }
    print $out $status;
    close $out;
}

# The main code ==============================================

# Parse command-line options
if (!getopts("dhc:o:") ||  $opt_h)
{
    usage();
    exit(0);
}
$config_name = $opt_c  if (length($opt_c) > 0);
$output_name = $opt_o  if (length($opt_o) > 0);

$config = parse_config_file($config_name, $opt_d);
update_status($config, $opt_d);
write_info($output_name);
