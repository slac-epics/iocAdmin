#!/usr/bin/perl -w
#
# ArchiveDaemon is a perl script that reads
# a configuration file which lists what Archive Engines
# should run on the local machine and how.
#
# It will start engines, maybe periodically stop and
# restart them.
# It provides a web server to display the status.
#
# All the ideas in here are adopted from Thomas Birke's
# tcl/tk CAManager/CAbgManager.
#
# kasemirk@ornl.gov

use English;
use strict;
use Socket;
use Cwd;
use Sys::Hostname;
use Time::Local qw(timelocal timelocal_nocheck);
use File::Basename;
use File::CheckTree;
use File::Path;
use IO::Handle;
use Data::Dumper;
use XML::Simple;
use POSIX 'setsid';
use vars qw($opt_h $opt_p $opt_f $opt_i $opt_u $opt_d);
use Getopt::Std;

# ----------------------------------------------------------------
# Configurables
# ----------------------------------------------------------------

# Compare: manual/changes.tex
my ($version) = "2.9.0";

# Setting this to 1 disables(!) caching and might help with debugging.
# Default: leave it commented-out.
# $OUTPUT_AUTOFLUSH=1;

# Log file, created in current directory.
# No good reason to change this one.
my ($logfile) = "ArchiveDaemon.log";

# Name of this host. Used to connect ArchiveEngines on this host,
# also part of the links on the daemon's web pages.
# Possible options:
# 1) Fixed string 'localhost'.
#    This will get you started, won't require DNS or any
#    other setup, and will always work for engines and web
#    clients on the local machine.
#    Won't work with web clients on other machines,
#    since they'll then follow links to 'localhost'.
# 2) Fixed string 'name.of.your.host'.
#    Will also always work plus allow web clients from
#    other machines on the network, but you have to assert
#    that your fixed string is correct.
# 3) hostname() function.
#    In theory, this is the best method.
#    In practice, it might fail when you have more than one
#    network card or no DNS.
#my ($host) = 'localhost';
#my ($host) = 'ics-srv-archive1';
my ($host) = hostname();

my ($localhost) = "127.0.0.1";

# What ArchiveEngine to use. Just "ArchiveEngine" works if it's in the path.
my ($ArchiveEngine) = "ArchiveEngine";

# Seconds between "is the engine running?" checks.
# We _hope_ that the engine is running all the time
# and only want to restart e.g. once a day,
# so checking every 30 seconds is more than enough
# yet gives us a fuzzy feeling that the daemon's web
# page shows the current state of things.
my ($engine_check_secper) = 30;

# A few seconds give good response yet daemon uses hardly any CPU.
my ($http_check_timeout) = 3;

# Timeout used when reading a HTTP client or ArchiveEngine.
# 10 seconds should be reasonable. Trying 30sec at SNS because
# the archive server is often very busy.
my ($read_timeout) = 30;

# Number of entries in the "Messages" log.
my ($message_queue_length) = 25;

# Detach from terminal etc. to run as a background daemon?
# 1 should always be OK unless you're doing strange debugging.
my ($daemonization) = 1;

# ----------------------------------------------------------------
# Globals
# ----------------------------------------------------------------

# The runtime configuration of this ArchiveDaemon, read via XMLin.
#
# Added tags for each $config->{engine}{$engine} that are
# adjusted at runtime:
# 'disabled'   => is there a file 'DISABLED.txt' ?
# 'lockfile'   => is there a file 'archive_active.lck' ?
# 'next_start' => next date/time when engine should start
# 'next_stop'  => next .... stop
# 'started'    => '' or start time info of the running engine.
# 'stopped'    => 1 if we stopped the engine, now waiting for lockfile to be removed.
# 'channels'   => # of channels (only valid if started)
# 'connected'  => # of _connected_ channels (only valid if started)
my ($config);

my ($daemon_path) = cwd();

my ($config_file) = "";

my (@message_queue);
my ($start_time_text);
my ($last_check) = 0;

# TODO: This looks bad.
my (%weekdays, @weekdays);
$weekdays{Su} = 0;
$weekdays{Mo} = 1;
$weekdays{Tu} = 2;
$weekdays{We} = 3;
$weekdays{Th} = 4;
$weekdays{Fr} = 5;
$weekdays{Sa} = 6;
$weekdays[0] = "Sunday";
$weekdays[1] = "Monday";
$weekdays[2] = "Tuesday";
$weekdays[3] = "Wednesday";
$weekdays[4] = "Thursday";
$weekdays[5] = "Friday";
$weekdays[6] = "Saturday";

# ----------------------------------------------------------------
# Message Queue
# ----------------------------------------------------------------

sub time_as_text($)
{
    my ($seconds) = @ARG;
    my ($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst)
        = localtime($seconds);
    return sprintf("%04d/%02d/%02d %02d:%02d:%02d",
           1900+$year, 1+$mon, $mday, $hour, $min, $sec);
}

sub time_as_short_text($)
{
    my ($seconds) = @ARG;
    my ($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst)
        = localtime($seconds);
    return sprintf("%04d_%02d_%02d-%02d_%02d_%02d",
           1900+$year, 1+$mon, $mday, $hour, $min, $sec);
}

sub add_message($)
{
    my ($msg) = @ARG;
    shift @message_queue if ($#message_queue >= $message_queue_length-1);
    push @message_queue, sprintf("%s: <I>%s</I>", time_as_text(time), $msg);
}

# ----------------------------------------------------------------
# Config File
# ----------------------------------------------------------------

# Reads config file into @config
sub read_config($)
{
    my ($file) = @ARG;
    $config = XMLin($file, ForceArray => [ 'engine' ], KeyAttr => 'directory' );
    print Dumper($config) if ($opt_d);

    my ($engine);
    foreach $engine ( keys %{ $config->{engine} } )
    {
        if ($config->{engine}{$engine}{restart}{type} eq "weekly")
        {
            my ($daytxt, $daily) = split(/ /, $config->{engine}{$engine}{restart}{content});
            my ($daychars) = substr($daytxt, 0, 2);
            die ("Engine '$engine': " .
                 "Unknown day name '$daytxt' " .
                 "in weekly restart config")
            unless defined($weekdays{$daychars});        
            # Hack weekly into daily, but only for one day of the week
            $config->{engine}{$engine}{restart}{type} = 'daily';
            $config->{engine}{$engine}{restart}{day} = $weekdays{$daychars};
            $config->{engine}{$engine}{restart}{content} = $daily;
        }            
        # Initialize the extra status stuff that's not in the config file.
        $config->{engine}{$engine}{disabled} = 0;
        $config->{engine}{$engine}{lockfile} = 0;
        $config->{engine}{$engine}{next_start} = 0;
        $config->{engine}{$engine}{next_stop} = 0;
        $config->{engine}{$engine}{started} = '';
        $config->{engine}{$engine}{stopped} = 0;
        $config->{engine}{$engine}{channels} = 0;
        $config->{engine}{$engine}{connected} = 0;
    }
}

# Update the start/stop times based on
# current state of the engine
# and the daily/hourly/... configuration.
sub update_schedule($)
{
    my ($now) = @ARG;
    my ($engine, $engine_start_secs);
    my ($n_sec,$n_min,$n_hour,$n_mday,$n_mon,$n_year,$n_wday,$x);
    my ($e_mon, $e_mday, $e_year, $e_hour, $e_min, $e_sec);
    my ($hour, $minute, $hours, $minutes);

    print(" ------ update_schedule  : ", time_as_text($now), "\n") if ($opt_d);
    ($n_sec,$n_min,$n_hour,$n_mday,$n_mon,$n_year,$n_wday,$x,$x) =
        localtime($now);
    foreach $engine ( keys %{ $config->{engine} } )
    {
        print(" Engine $engine ") if ($opt_d);
        # Get engine_start_secs
        if (length($config->{engine}{$engine}{started}) > 0)
        {
            ($e_mon, $e_mday, $e_year, $e_hour, $e_min, $e_sec)
                = split '[/ :.]', $config->{engine}{$engine}{started};
            $engine_start_secs
                = timelocal($e_sec,$e_min,$e_hour,$e_mday,
                            $e_mon-1,$e_year-1900);
            print("  Started: $config->{engine}{$engine}{started} = ",
                  time_as_text($engine_start_secs), "\n") if ($opt_d);
        }
        else
        {
            print("  Not running\n") if ($opt_d);
            $engine_start_secs = 0;
        }
        # Determine next_start, next_stop
        if ($config->{engine}{$engine}{restart}{type} eq 'daily')
        {   
            if ($engine_start_secs <= 0)
            {   # Not Running: Start as soon as possible
                $config->{engine}{$engine}{next_start} = $now;
                $config->{engine}{$engine}{next_stop}  = 0;
            }
            else
            {   # Running: Determine stop time
                ($hour, $minute) = split ':',
                            $config->{engine}{$engine}{restart}{content};
                $config->{engine}{$engine}{next_start} = 0;
                if (defined($config->{engine}{$engine}{restart}{day}))
                {   # Target: $engine->{day}; Today's daynum: $n_wday
                    my ($days_to_go) =
                    $config->{engine}{$engine}{restart}{day} - $n_wday;
                    $days_to_go += 7 if ($days_to_go < 0);
                    $config->{engine}{$engine}{next_stop} =
                    timelocal_nocheck(0, $minute,$hour,
                      $n_mday+$days_to_go,$n_mon, $n_year);
                    # Already happened today, so we'll try again next week?
                    $config->{engine}{$engine}{next_stop} += 24*60*60*7
                        if ($engine_start_secs >
                                    $config->{engine}{$engine}{next_stop});
                }
                else
                {
                    $config->{engine}{$engine}{next_stop} = timelocal(0,$minute,$hour,
                                 $n_mday,$n_mon,$n_year);
                    # Today or already into the next day?
                    $config->{engine}{$engine}{next_stop} += 24*60*60
                    if ($engine_start_secs > $config->{engine}{$engine}{next_stop});
                }
            }
        }
        elsif ($config->{engine}{$engine}{restart}{type} eq "hourly")
        {
            if ($engine_start_secs <= 0)
            {   # Not Running: Start as soon as possible
                $config->{engine}{$engine}{next_start} = $now;
                $config->{engine}{$engine}{next_stop}  = 0;
            }
            else
            {   # Running: Determine stop time (relative to engine)
                my ($rounding) = int($config->{engine}{$engine}{restart}{content} * 60*60);
                $config->{engine}{$engine}{next_start} = 0;
                $config->{engine}{$engine}{next_stop} =
                    (int($engine_start_secs/$rounding)+1) * $rounding;
            }
        }
        elsif ($config->{engine}{$engine}{restart}{type} eq "timed")
        {   
            ($hour, $minute, $hours, $minutes) =
                  split '[:/]',$config->{engine}{$engine}{restart}{content};
            # Calc start/end
            my ($start) = timelocal(0,$minute,$hour,$n_mday,$n_mon,$n_year);
            my ($stop) = $start + ($hours*60 + $minutes) * 60;
            if ($engine_start_secs > 0)
            {   # Running: Observe stop time
                $config->{engine}{$engine}{next_start} = 0;
                $config->{engine}{$engine}{next_stop} = $stop;
            }
            elsif ($start <= $now  and  $now <= $stop)
            {   # Not running but should
                $config->{engine}{$engine}{next_start} = $start;
                $config->{engine}{$engine}{next_stop} = 0;
            }
            else
            {   # Not running and shouldn't
                $config->{engine}{$engine}{next_start} = 0;
                $config->{engine}{$engine}{next_stop} = 0;
            }
        }
        else
        {   
            if ($engine_start_secs <= 0)
            {   # Not Running: Start as soon as possible
                $config->{engine}{$engine}{next_start} = $now;
                $config->{engine}{$engine}{next_stop}  = 0;
            }
            else
            {   # Running: keep going w/o end
                $config->{engine}{$engine}{next_start} = $config->{engine}{$engine}{next_stop} = 0;
            }
        }
        if ($opt_d)
        {
            print "   Next start: ", time_as_text($config->{engine}{$engine}{next_start}), "\n"
                if ($config->{engine}{$engine}{next_start} > 0);
            print "   Next stop : ", time_as_text($config->{engine}{$engine}{next_stop}),  "\n"
                if ($config->{engine}{$engine}{next_stop} > 0);
        }
    }
}

# ----------------------------------------------------------------
# HTTPD Stuff
# ----------------------------------------------------------------

# Stuff prepended/appended to this HTTPD's HTML pages.
sub html_start($$)
{
    my ($client, $refresh) = @ARG;
    my ($web_refresh) = $engine_check_secper / 2;
    print $client "<HTML>\n";
    print $client "<META HTTP-EQUIV=\"Refresh\" CONTENT=$web_refresh>\n"
    if ($refresh);
    print $client "<HEAD>\n";
    print $client "<TITLE>Archive Daemon</TITLE>\n";
    print $client "</HEAD>\n";
    print $client "<BODY BGCOLOR=#A7ADC6 LINK=#0000FF " .
    "VLINK=#0000FF ALINK=#0000FF>\n";
    print $client "<FONT FACE=\"Helvetica, Arial\">\n";
    print $client "<BLOCKQUOTE>\n";
}

sub html_stop($)
{
    my ($client) = @ARG;
    print $client "</BLOCKQUOTE>\n";
    print $client "</FONT>\n";
    print $client "<P><HR WIDTH=50% ALIGN=LEFT>\n";
    print $client "<A HREF=\"/\">-Engines-</A>\n";
    print $client "<A HREF=\"/info\">-Info-</A>  \n";
    print $client time_as_text(time);
    print $client "<br>\n";
    print $client "<I>Note that this page updates only ";
    print $client "every $engine_check_secper seconds!</I>\n";
    print $client "</BODY>\n";
    print $client "</HTML>\n";
}

# $httpd_handle = create_HTTPD($tcp_port)
#
# Call once to create HTTPD.
#
sub create_HTTPD($)
{
    my ($port) = @ARG;
    my ($EOL) = "\015\012";
    socket(HTTP, PF_INET, SOCK_STREAM, getprotobyname('tcp')) 
        or die "Cannot create socket: $!\n";
    setsockopt(HTTP, SOL_SOCKET, SO_REUSEADDR, pack("l", 1))
        or die "setsockopt: $!";
    bind(HTTP, sockaddr_in($port, INADDR_ANY))
        or die "bind: $!";
    listen(HTTP,SOMAXCONN) or die "listen: $!";
    print("Server started on port $port\n");
    return \*HTTP;
}

sub handle_HTTP_main($)
{
    my ($client) = @ARG;
    my ($engine, $message, $channels, $connected);
    html_start($client, 1);
    print $client "<H1>Archive Daemon</H1>\n";
    print $client "<TABLE BORDER=0 CELLPADDING=5>\n";
    print $client "<TR>";
    # TABLE:   Engine  |  Port  |  Started  |  Status  |  Restart | Action
    print $client "<TH BGCOLOR=#000000><FONT COLOR=#FFFFFF>Engine</FONT></TH>";
    print $client "<TH BGCOLOR=#000000><FONT COLOR=#FFFFFF>Port</FONT></TH>";
    print $client "<TH BGCOLOR=#000000><FONT COLOR=#FFFFFF>Started</FONT></TH>";
    print $client "<TH BGCOLOR=#000000><FONT COLOR=#FFFFFF>Status</FONT></TH>";
    print $client "<TH BGCOLOR=#000000><FONT COLOR=#FFFFFF>Restart</FONT></TH>";
    print $client "<TH BGCOLOR=#000000><FONT COLOR=#FFFFFF>Action</FONT></TH>";
    print $client "</TR>\n";
    foreach $engine ( keys %{ $config->{engine} } )
    {
        print $client "<TR>";
        # Engine link
        print $client "<TH BGCOLOR=#FFFFFF>" .
            "<A HREF=\"http://$host:$config->{engine}{$engine}{port}\">" .
            "$config->{engine}{$engine}{desc}</A></TH>";
        # Port
        print $client "<TD ALIGN=CENTER>$config->{engine}{$engine}{port}</TD>";
        # Started | Status
        if (length($config->{engine}{$engine}{started}) > 0)
        {
            print $client "<TD ALIGN=CENTER>$config->{engine}{$engine}{started}</TD>";
            $connected = $config->{engine}{$engine}{connected};
            $channels  = $config->{engine}{$engine}{channels};
            print $client "<TD ALIGN=CENTER>";
            print $client "<FONT COLOR=#FF0000>" if ($channels != $connected);
            print $client "$connected/$channels channels connected";
            print $client "</FONT>" if ($channels != $connected);
            if ($config->{engine}{$engine}{disabled})
            {
                print $client "<br><FONT COLOR=#FF0000>";
                print $client "Found DISABLED file.";
                print $client "</FONT>";
            }
            print $client "</TD>";
        }
        elsif ($config->{engine}{$engine}{stopped})
        {
            print $client "<TD></TD>";
            print $client "<TD ALIGN=CENTER><FONT color=#FFFF00>" .
                "Stopped, waiting for shutdown.</FONT></TD>";
        }
        else
        {
            print $client "<TD></TD>";
            if ($config->{engine}{$engine}{lockfile})
            {
                print $client "<TD ALIGN=CENTER><FONT color=#FF0000>" .
                    "Unknown. Lock file, but no response.</FONT><br>";
                print $client "(Check again, see if issue persists)</TD>";
            }
            elsif ($config->{engine}{$engine}{disabled})
            {
                print $client "<TD ALIGN=CENTER><FONT color=#FFFF00>" .
                    "Disabled.</FONT></TD>";
            }
            else
            {
                print $client "<TD ALIGN=CENTER><FONT color=#FF0000>" .
                    "Not running.</FONT>";
            }
        }
        # Restart
        print $client "<TD ALIGN=CENTER>";
        if ($config->{engine}{$engine}{restart}{type} eq "daily")
        {
            if (defined($config->{engine}{$engine}{restart}{day}))
            {
                print $client "Every $weekdays[$config->{engine}{$engine}{restart}{day}] " .
                    "at $config->{engine}{$engine}{restart}{content}.";
            }
            else
            {
                print $client "Daily at $config->{engine}{$engine}{restart}{content}.";
            }
        }
        elsif ($config->{engine}{$engine}{restart}{type} eq "hourly")
        {
            print $client "Every $config->{engine}{$engine}{restart}{content} h.";
        }
        elsif ($config->{engine}{$engine}{restart}{type} eq "timed")
        {
            print $client "Start/Duration: $config->{engine}{$engine}{restart}{content}";
        }
        else
        {
            print $client "Continuously running.";
        }
        if ($config->{engine}{$engine}{next_start} > 0)
        {
            print $client
                "<br>Next start ", time_as_text($config->{engine}{$engine}{next_start});
        }
        if ($config->{engine}{$engine}{next_stop} > 0)
        {
            print $client
                "<br>Next stop ", time_as_text($config->{engine}{$engine}{next_stop});
        }
        print $client "</TD>";
        # Action
        if ($config->{engine}{$engine}{disabled})
        {
            print $client
                "<TD><A HREF=\"enable/$config->{engine}{$engine}{port}\">Enable</A></TD>";
        }
        else
        {
            print $client
                "<TD><A HREF=\"disable/$config->{engine}{$engine}{port}\">Disable</A></TD>";
        }
    
        print $client "</TR>\n";
    }
    print $client "</TABLE>\n";
    print $client "<H2>Messages</H2>\n";
    foreach $message ( reverse @message_queue )
    {
        print $client "$message<BR>\n";
    }
    html_stop($client);
}

sub handle_HTTP_status($)
{
    my ($client) = @ARG;
    my ($engine, $total, $disabled, $running, $channels, $connected);
    html_start($client, 1);
    print $client "<H1>Archive Daemon Status</H1>\n";
    print $client "# Description|Port|Running|Connected|Channels<BR>\n";  
    $total = 0;
    $disabled = $running = $channels = $connected = 0;
    foreach $engine ( keys %{ $config->{engine} } )
    {
        ++$total;
        print $client "ENGINE $config->{engine}{$engine}{desc}|";
        print $client "$config->{engine}{$engine}{port}|";
        if ($config->{engine}{$engine}{disabled})
        {
            print $client "disabled|0|0<br>\n";
            ++$disabled;
        }
        elsif (length($config->{engine}{$engine}{started}) > 0)
        {
            ++$running;
            $channels += $config->{engine}{$engine}{channels};
            $connected += $config->{engine}{$engine}{connected};
            print $client "running|$config->{engine}{$engine}{connected}|$config->{engine}{$engine}{channels}<br>\n";
        }
        else
        {
            print $client "down|0|0<br>\n";
        }
    }
    print $client "# engines|disabled|running|connected|channels<BR>\n";  
    print $client "#TOTAL $total|$disabled|$running|$connected|$channels<BR>\n";  
    html_stop($client);
}

sub handle_HTTP_info($)
{
    my ($client) = @ARG;
    html_start($client, 1);
    print $client "<H1>Archive Daemon Info</H1>\n";
    print $client "<TABLE BORDER=0 CELLPADDING=5>\n";

    print $client "<TR><TD><B>Version:</B></TD><TD>$version</TD></TR>\n";
    print $client "<TR><TD><B>Config File:</B></TD><TD>$config_file</TD></TR>\n";
    print $client "<TR><TD><B>Daemon start time:</B></TD><TD> $start_time_text</TD></TR>\n";
    print $client "<TR><TD><B>Check Period:</B></TD><TD> $engine_check_secper secs</TD></TR>\n";
    print $client "<TR><TD><B>Last Check:</B></TD><TD> ", time_as_text($last_check), "</TD></TR>\n";
    print $client "</TABLE>\n";
    html_stop($client);
}

sub handle_HTTP_postal($)
{
    my ($client) = @ARG;
    my ($engine);
    html_start($client, 0);
    print $client "<H1>Postal Archive Daemon</H1>\n";
    foreach $engine ( keys %{ $config->{engine} } )
    {
        next unless ($config->{engine}{$engine}{started});
        print $client "Stopping " .
            "<A HREF=\"http://$host:$config->{engine}{$engine}{port}\">" .
            "$config->{engine}{$engine}{desc}</A> on port $config->{engine}{$engine}{port}<br>\n";
        stop_engine($config->{engine}{$engine}{port});
    }
    print $client "Quitting<br>\n";
    html_stop($client);
}

sub handle_HTTP_disable($$)
{
    my ($client, $engine_port) = @ARG;
    my ($engine);
    html_start($client, 0);
    print $client "<H1>Disabling Engines</H1>\n";
    foreach $engine ( keys %{ $config->{engine} } )
    {
        if ($config->{engine}{$engine}{port} == $engine_port)
        {
            print $client "<H2>Engine '$config->{engine}{$engine}{desc}'</H2>\n";
            if (open(DISABLED, ">$engine/DISABLED.txt"))
            {
                print DISABLED "Disabled by ArchiveDaemon, ";
                print DISABLED time_as_text(time);
                print DISABLED "\n";
                close(DISABLED);
                print $client "Created 'DISABLED.txt' in '$engine'.<p>\n";
                stop_engine($config->{engine}{$engine}{port});
                $config->{engine}{$engine}{disabled} = 1;
                print $client "Stopped via port $config->{engine}{$engine}{port}.\n";
                print "Disabled '$config->{engine}{$engine}{desc}, port $config->{engine}{$engine}{port}.\n";
            }
            else
            {
                print $client "ERROR: Cannot create 'DISABLED.txt' in '$engine' directory.";
            }
            last;
        }
    }
    html_stop($client);
}

sub handle_HTTP_enable($$)
{
    my ($client, $engine_port) = @ARG;
    my ($engine, $dir);
    html_start($client, 0);
    print $client "<H1>Enabling Engines</H1>\n";
    foreach $engine ( keys %{ $config->{engine} } )
    {
        if ($config->{engine}{$engine}{port} == $engine_port)
        {
            print $client "<H2>Engine '$config->{engine}{$engine}{desc}'</H2>\n";
            if (unlink("$engine/DISABLED.txt") == 1)
            {
                $config->{engine}{$engine}{disabled} = 0;
                $last_check = 0; # Force immediate check of engines & maybe restart
                print $client "Removed 'DISABLED.txt' from '$engine'.<p>\n";
                print $client "Daemon will eventually start the engine ";
                print $client "at the next check interval.\n";
                print "Enabled '$config->{engine}{$engine}{desc}, port $config->{engine}{$engine}{port}.\n";
            }
            else
            {
                print $client "ERROR: Cannot find 'DISABLED.txt' in '$engine'.";
            }
            last;
        }
    }
    html_stop($client);
}

# Used by check_HTTPD to dispatch requests
sub handle_HTTP_request($$)
{
    my ($doc, $client) = @ARG;
    my ($URL);
    if ($doc =~ m/GET (.+) HTTP/)
    {
        $URL = $1;
        if ($URL eq '/')
        {
            handle_HTTP_main($client);
            return 1;
        }
        elsif ($URL eq '/status')
        {
            handle_HTTP_status($client);
            return 1;
        }
        elsif ($URL eq '/info')
        {
            handle_HTTP_info($client);
            return 1;
        }
        elsif ($URL eq '/stop')
        {
            html_start($client, 0);
            print $client "Quitting.\n";
            html_stop($client);
            return 0;
        }
        elsif ($URL eq '/postal')
        {
            handle_HTTP_postal($client);
            return 0;
        }
        elsif ($URL =~ m'/disable/([0-9]+)')
        {
            handle_HTTP_disable($client, $1);
            return 1;
        }
        elsif ($URL =~ m'/enable/([0-9]+)')
        {
            handle_HTTP_enable($client, $1);
            return 1;
        }
        else
        {
            print $client "<H1>Unknown URL</H1>\n";
            print $client "You requested: '<I>" . $URL . "</I>',\n";
            print $client "but this URL is not handled by the ArchiveDaemon.\n";
            return 1;
        }
    }
    print $client "<B>Error</B>\n";
    return 1;
}

# check_HTTPD($httpd_handle)
#
# Call periodically to check for HTTP clients
#
sub check_HTTPD($)
{
    my ($sock) = @ARG;
    my ($c_addr, $smask, ,$smask_in, $num);
    $smask_in = '';
    vec($smask_in, fileno($sock), 1) = 1;
    $num = select($smask=$smask_in, undef, undef, $http_check_timeout);
    return if ($num <= 0);
    if ($c_addr = accept(CLIENT, $sock))
    {
        #my($c_port,$c_ip) = sockaddr_in($c_addr);
        #print "HTTP Client ", inet_ntoa($c_ip), ":", $c_port, "\n";
        # Read the client's request
        my ($mask, ,$mask_in, $doc, $line);
        $doc = '';
        $mask_in = '';
        vec($mask_in, fileno(CLIENT), 1) = 1;
        $num = select($mask=$mask_in, undef, undef, $read_timeout);
        while ($num > 0)
        {
            $num = sysread CLIENT, $line, 5000;
            last if (not defined($num));
            last if ($num == 0);
            $line =~ s[\r][]g;
            $doc = $doc . $line;
            #print("Line: ($num) '$line'\n");
            #print("Doc: '$doc'\n");
            if ($doc =~ m/\n\n/s)
            {
                #print "Found end of request\n";
                last;
            }
            $num = select($mask=$mask_in, undef, undef, $read_timeout);
        }
        # Respond
        print CLIENT "HTTP/1.1 200 OK\n";
        print CLIENT "Server: Apache/2.0.40 (Red Hat Linux)\n";
        print CLIENT "Connection: close\n";
        print CLIENT "Content-Type: text/html\n";
        print CLIENT "\n"; 
        my ($continue) = handle_HTTP_request($doc, \*CLIENT);
        close CLIENT;
        if (not $continue)
        {
            print("Quitting.\n");
            exit(0);
        }
    }
}    

# ----------------------------------------------------------------
# Engine Start/Stop Stuff
# ----------------------------------------------------------------

# Return what the given engine should use for an index name,
# including the relative path with YYYY/MM_DD etc. if applicable.
sub make_indexname($$)
{
    my ($now, $engine) = @ARG;
    my ($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst)
    = localtime($now);
    if ($config->{engine}{$engine}{restart}{type} eq "hourly")
    {
        return sprintf("%04d/%02d_%02d_%02dh/index",
                   1900+$year, 1+$mon, $mday, $hour);
    }
    if ($config->{engine}{$engine}{restart}{type} eq "daily"  or
        $config->{engine}{$engine}{restart}{type} eq "timed")
    {
        return sprintf("%04d/%02d_%02d/index", 1900+$year, 1+$mon, $mday);
    }
    return "index";
}

# Connects to HTTPD at host/port and reads a URL,
# returning the raw document.
sub read_URL($$$)
{
    my ($host, $port, $URL) = @ARG;
    my ($ip, $addr, $EOL);
    $EOL = "\015\012";
    $ip = inet_aton($host) or die "Invalid host: $host";
    $addr = sockaddr_in($port, $ip);
    socket(SOCK, PF_INET, SOCK_STREAM, getprotobyname('tcp')) 
    or die "Cannot create socket: $!\n";
    connect(SOCK, $addr)  or  return "";
    autoflush SOCK 1;    
    print SOCK "GET $URL HTTP/1.0$EOL";
    print SOCK "Accept: text/html, text/plain$EOL";
    print SOCK "User-Agent: perl$EOL";
    print SOCK "$EOL";
    print SOCK "$EOL";
    my ($mask, $num, $line, @doc);
    $mask = '';
    vec($mask, fileno(SOCK), 1) = 1;
    $num = select($mask, undef, undef, $read_timeout);
    while ($num > 0  and not eof SOCK)
    {
        $line = <SOCK>;
        push @doc, $line;
        chomp $line;
        $mask = '';
        vec($mask, fileno(SOCK), 1) = 1;
        $num = select($mask, undef, undef, $read_timeout);
    }
    close (SOCK);
    return @doc;
}    

# Test if ArchiveEngine runs on port,
# returning (description, start time, # channels, # connected channels)
sub check_engine($)
{
    my ($port) = @ARG;
    my ($line, $desc, $started, $channels, $connected);
    $desc = '';
    $started = '';
    $channels = $connected = 0;
    foreach $line ( read_URL($localhost, $port, "/") )
    {
        if ($line =~ m'Description</TH>.*>([^>]+)</TD>')
        {
            $desc = $1;
        }
        elsif ($line =~ m'Started</TH>.+(\d{2}/\d{2}/\d{4} \d{2}:\d{2}:\d{2})')
        {   #     Compare: Started</TH>... 03/17/2004 18:01:01.208511000
            $started = $1;
        }
        elsif ($line =~ m'Channels</TH>[^0-9]+([0123456789]+)')
        {
            $channels = $1;
        }
        elsif ($line =~ m'Connected</TH>.+>([0123456789]+)<')
        {
            $connected = $1;
        }
    }
    return ($desc, $started, $channels, $connected);
}

# Query all engines in config, check if they're running
sub check_engines($)
{
    my ($now) = @ARG;
    my ($engine, $desc, $started, $channels, $connected);
    print(" -------------------- check_engines\n") if ($opt_d);
    foreach $engine ( keys %{ $config->{engine} } )
    {
        # Check some marker files.
        $config->{engine}{$engine}{disabled} = (-f "$engine/DISABLED.txt");
        $config->{engine}{$engine}{lockfile} = (-f "$engine/archive_active.lck");
        # Did we just stop the engine?
        if ($config->{engine}{$engine}{stopped})
        {
            if ($config->{engine}{$engine}{lockfile})
            {
                # There is still a lock file, assume engine has not yet quit.
                print("$config->{engine}{$engine}{desc} still shutting down\n") if ($opt_d);
                $config->{engine}{$engine}{started} = '';
                $config->{engine}{$engine}{channels} = 0;
                $config->{engine}{$engine}{connected} = 0;           
                next; # Don't bother checking 
            }
            # else: Looks like engine removed the lockfile, so it stopped 100%
            print("$config->{engine}{$engine}{desc} shut downdown completed.\n") if ($opt_d);
            $config->{engine}{$engine}{stopped} = 0;
        }
        # Query engine's HTTPD.
        ($desc, $started, $channels, $connected) =
            check_engine($config->{engine}{$engine}{port});
        if (length($started) > 0)
        {
            $config->{engine}{$engine}{started} = $started;
            $config->{engine}{$engine}{channels} = $channels;
            $config->{engine}{$engine}{connected} = $connected;
            print("$config->{engine}{$engine}{desc} running since $config->{engine}{$engine}{started}\n")
                if ($opt_d);
        }
        else
        {
            print("$config->{engine}{$engine}{desc} NOT running\n")
                if ($opt_d);
            $config->{engine}{$engine}{started} = '';
        }
    }
}

# Stop ArchiveEngine on port
# Returns '1' if we got the 'OK' from the engine,
# 0 if we have no idea what'll happen.
sub stop_engine($)
{
    my ($port) = @ARG;
    my (@doc, $line);
    print("Stopping engine $localhost:$port\n");
    add_message("Stopping engine $localhost:$port");
    @doc = read_URL($localhost, $port, "/stop");
    foreach $line ( @doc )
    {
        return 1 if ($line =~ m'Engine will quit');
    }
    print(time_as_text(time),
      ": Engine $localhost:$port won't quit.\nResponse : @doc\n");
    return 0;
}

# For all engines, check if now's the time to stop it,
# and if so, do it.
sub stop_engines($)
{
    my ($now) = @ARG;
    my ($engine);
    my ($stopped) = 0;
    print(" ---- stop engines\n") if ($opt_d);
    foreach $engine ( keys %{ $config->{engine} } )
    {
        next if ($config->{engine}{$engine}{next_stop} <= 0); # never
        next if ($config->{engine}{$engine}{next_stop} > $now); # not, yet

        print "$config->{engine}{$engine}{desc} to stop at ",
               time_as_text($config->{engine}{$engine}{next_stop}),  "\n"
            if ($opt_d);

        stop_engine($config->{engine}{$engine}{port});
        # Mark engine as not running, and stopped on purpose.
        $config->{engine}{$engine}{started} = '';
        $config->{engine}{$engine}{stopped} = 1;
        ++ $stopped;
    }
    return $stopped;
}

# Attempt to start one engine
# Returns 1 if the master index config. was updated
sub start_engine($$)
{
    my ($now, $engine) = @ARG;
    my ($null) = "/dev/null";

    # $index = "2006/01_15/index"
    my ($index) = make_indexname($now, $engine);
    add_message(
       "Starting Engine '$config->{engine}{$engine}{desc}': $host:$config->{engine}{$engine}{port}\n");
    my ($new_data_path) = dirname("$engine/$index");
    if (not -d $new_data_path)
    {
        add_message("Creating dir '$new_data_path'\n");
        mkpath($new_data_path);
    }
    my ($EngineLog) = time_as_short_text($now) . ".log";
    my ($EngineOut) = time_as_short_text($now) . ".out";
    my ($cmd) = "cd \"$engine\";" .
        "$ArchiveEngine -d \"$config->{engine}{$engine}{desc}\" -l $EngineLog " .
        "-p $config->{engine}{$engine}{port} $config->{engine}{$engine}{config} $index >$EngineOut 2>&1 &";
    print(time_as_text(time), ": Command: '$cmd'\n");
    system($cmd);
    # Create symlink to the new index ('2005/03_21/index')
    # from 'current index' in the engine directory.
    my ($current) = readlink("$engine/current_index");
    if (defined($current) && ($current ne $index))
    {
        # Inform index mechanism about the old 'current' sub-archive
        # that's now complete, but only if that's really an older one.
        # In case the new 'current_index' matches the one found,
        # we should not yet copy/update anything.
        if (exists($config->{engine}{$engine}{dataserver}{host}))
        {
            my ($datadir) = dirname($current);
            my ($info);
            if ($host =~ m/$config->{engine}{$engine}{dataserver}{host}/)
            {   # On same host: Just information.
                $info = "new $host:$daemon_path/$engine/$datadir";
            }
            else
            {   # On remote host: need source, target info for copy.
                $info = "copy $host:$daemon_path/$engine/$datadir "
                . "$config->{engine}{$engine}{dataserver}{host}:$daemon_path/$engine/$datadir";
            }
            if (exists($config->{mailbox}))
            {
                my ($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst)
                       = localtime($now);
                my ($info_name) = sprintf("%s/%s-%04d_%02d_%02d-%02d_%02d_%02d.txt",
                                        $config->{mailbox}, $engine, 1900+$year, 1+$mon, $mday,
                                        $hour, $min, $sec);
                if (open(INFO_FILE, ">$info_name"))
                {
                    print INFO_FILE "$info\n";
                    close INFO_FILE;
                }
                print"$info\n";
            }
            else
            {
                print"No <mailbox> for $info\n";
            }
        }
        # if link exists, remove it
        unlink("$engine/current_index");
    }
    symlink("$index", "$engine/current_index");
    print("symlink to $index\n");
    return 0;
}

# Attempt to start all engines that have their start time set
sub start_engines($)
{
    my ($now) = @ARG;
    my ($engine);
    print(" ---- start engines ", time_as_text($now),"\n") if ($opt_d);
    foreach $engine ( keys %{ $config->{engine} } )
    {
        next if ($config->{engine}{$engine}{disabled});               # ignore while disabled
        next unless ($config->{engine}{$engine}{next_start} > 0);     # not scheduled
        next unless ($config->{engine}{$engine}{next_start} <= $now); # not due, yet.
        if ($config->{engine}{$engine}{stopped})
        {
            print "Engine $config->{engine}{$engine}{desc} still quitting?\n";
            next;
        }
        if ($config->{engine}{$engine}{lockfile})
        {
            print "Cannot start engine $config->{engine}{$engine}{desc}: locked\n";
            next;
        }
        start_engine($now, $engine);
    }
}

# ----------------------------------------------------------------
# Main
# ----------------------------------------------------------------
sub usage()
{
    print("USAGE: ArchiveDaemon [options]\n");
    print("\n");
    print("Options:\n");
    print("\t-f <file>    : config. file\n");
    print("\t-d           : debug mode (stay in foreground etc.)\n");
    print("\n");
    print("This tool automatically starts, monitors and restarts\n");
    print("ArchiveEngines based on a config. file.\n");
    exit(-1);
}
if (!getopts('hf:d')  ||  $#ARGV != -1  ||  $opt_h)
{
    usage();
}
if ($opt_d)
{
    print("Debug mode, will run in foreground\n");
    $daemonization = 0;
}

# Allow command line options to override various defaults
$config_file = $opt_f if ($opt_f);
if (length($config_file) <= 0)
{
    print("Need config file (option -f):\n");
    usage();
}
read_config($config_file);

add_message("Started");
print("Read $config_file. Check status via\n");
print("          http://$host:$config->{port}\n");
print("You can also monitor the log file:\n");
print("          $logfile\n");
# Daemonization, see "perldoc perlipc"
if ($daemonization)
{
    print("Disassociating from terminal.\n");
    open STDIN, "/dev/null" or die "Cannot disassociate STDIN\n";
    open STDOUT, ">$logfile" or die "Cannot create $logfile\n";
    defined(my $pid = fork) or die "Can't fork: $!";
    exit if $pid;
    setsid                  or die "Can't start a new session: $!";
    open STDERR, '>&STDOUT' or die "Can't dup stdout: $!";
}
$start_time_text = time_as_text(time);
my ($httpd) = create_HTTPD($config->{port});
my ($now);
while (1)
{
    $now = time;
    if (($now - $last_check) > $engine_check_secper)
    {
    print("\n############## Main Loop Check ####\n") if ($opt_d);
    check_engines($now);
    update_schedule($now);
    start_engines($now);
    if (stop_engines($now) > 0)
    {
        # We stopped engines. Check again soon for restarts.
        $last_check += 10;
    }
    else
    {
        $last_check = time;
    }
    }
    check_HTTPD($httpd);
}

