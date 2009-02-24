#!/usr/bin/perl
# make_archive_web.pl
#
# Create web page with archive info
# (daemons, engines, ...)
# from tab-delimited configuration file.

BEGIN
{
    push(@INC, 'scripts' );
    push(@INC, '/arch/scripts' );
}

use English;
use strict;
use vars qw($opt_d $opt_h $opt_c $opt_o);
use Getopt::Std;
use Data::Dumper;
use Sys::Hostname;
use archiveconfig;

# Globals, Defaults
my ($config_name) = "archiveconfig.xml";
my ($output_name) = "archive_status.html";
my ($localhost) = hostname();

# Configuration info filled by parse_config_file
my ($config);

sub usage()
{
    print("USAGE: make_archive_web [options]\n");
    print("\n");
    print("Options:\n");
    print(" -h          : help\n");
    print(" -c <config> : Use given config file instead of $config_name\n");
    print(" -o <html>   : Generate given html file instead of $output_name\n");
    print(" -d          : debug\n");
}

sub write_html($)
{
    my ($filename) = @ARG;
    my $TITLE = "ARCHIVER HEALTH STATUS";
    my ($d_dir, $e_dir);
    my ($out, $disconnected);
    open($out, ">$filename") or die "Cannot open '$filename'\n";

    print $out <<"XML";
<html>
<head></head>

<body BGCOLOR=#A7ADC6 LINK=#0000FF VLINK=#0000FF ALINK=#0000FF>

<H1 align=center>$TITLE</H1>

<table border="1" cellpadding="1" cellspacing="1" style'"border-collapse: collapse"
 bordercolor="#111111" bgcolor="#CCCCFF" width="100%"'>

  <tr>
     <td width="15%" align="center"><b>DAEMON</b></td>
     <td width="10%" align="center"><b>ENGINE</b></td>
     <td width="5%"  align="center"><b>PORT</b></td>
     <td width="20%" align="center"><b>DESCRIPTION</b></td>
     <td width="35%" align="center"><b>STATUS</b></td>
     <td width="10%" align="center"><b>RESTART</b></td>
     <td width="5%" align="center"><b>TIME</b></td>
  </tr>
XML

    foreach $d_dir ( sort keys %{ $config->{daemon} } )
    {
        next unless is_localhost($config->{daemon}{$d_dir}{'run'});
        print $out "  <tr>\n";
        print $out "     <td width=\"15%\">"
                   . "<A HREF=\"http://$localhost:$config->{daemon}{$d_dir}{port}\">$d_dir</A></td>\n";
        print $out "     <td width=\"10%\">&nbsp;</td>\n";
        print $out "     <td width=\"5%\">$config->{daemon}{$d_dir}{port}</td>\n";
        print $out "     <td width=\"20%\">$config->{daemon}{$d_dir}{desc}</td>\n";
        if ($config->{daemon}{$d_dir}{status} eq "running")
        {
            print $out "     <td width=\"35%\">Running</td>\n";
        }
        else
        {
            print $out "     <td width=\"35%\">" .
                      "<FONT color=#FF0000>$config->{daemon}{$d_dir}{status}</FONT></td>\n";
        }
        print $out "     <td width=\"5%\">&nbsp;</td>\n";
        print $out "     <td width=\"10%\">&nbsp;</td>\n";
        print $out "  </tr>\n";
        foreach $e_dir ( sort keys %{ $config->{daemon}{$d_dir}{engine} } )
        {
            next unless
                 is_localhost($config->{daemon}{$d_dir}{engine}{$e_dir}{'run'});
            print $out "  <tr>\n";
            print $out "     <td width=\"15%\">&nbsp;</td>\n";
            print $out "     <td width=\"10%\">" .
                       "<A HREF=\"http://$localhost:$config->{daemon}{$d_dir}{engine}{$e_dir}{port}\">$e_dir</A></td>\n";
            print $out "     <td width=\"5%\">$config->{daemon}{$d_dir}{engine}{$e_dir}{port}</td>\n";
            print $out "     <td width=\"20%\">$config->{daemon}{$d_dir}{engine}{$e_dir}{desc}</td>\n";
            if ($config->{daemon}{$d_dir}{engine}{$e_dir}{status} eq "running")
            {
                $disconnected =  $config->{daemon}{$d_dir}{engine}{$e_dir}{channels}
                     - $config->{daemon}{$d_dir}{engine}{$e_dir}{connected};
                if ($disconnected == 0)
                {
                    print $out "     <td width=\"35%\">$config->{daemon}{$d_dir}{engine}{$e_dir}{channels} channels connected.</td>\n";
                }
                else
                {
                    print $out "     <td width=\"35%\">$config->{daemon}{$d_dir}{engine}{$e_dir}{channels} channels, <FONT color=#FF0000>$disconnected disconnected</FONT>.</td>\n";
                }
            }
            else
            {
                print $out "     <td width=\"35%\"><FONT color=#FF0000>$config->{daemon}{$d_dir}{engine}{$e_dir}{status}</FONT></td>\n";
            }
            print $out "     <td width=\"10%\">$config->{daemon}{$d_dir}{engine}{$e_dir}{restart}{type}</td>\n";
            print $out "     <td width=\"5%\">$config->{daemon}{$d_dir}{engine}{$e_dir}{restart}{content}</td>\n";
            print $out "  </tr>\n";
        }
    }
    print $out "<hr><p>\n";
    print $out "Last Update: ", time_as_text(time), "<p>\n";
    print $out "</table>\n";
    print $out "</body>\n";
    print $out "</html>\n";
 
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

write_html($output_name);
