#!/usr/bin/perl

BEGIN
{
    push(@INC, 'scripts' );
    push(@INC, '/arch/scripts' );
}

use English;
use strict;
use vars qw($opt_d $opt_h $opt_c $opt_s);
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
    print("USAGE: update_archive_tree [options]\n");
    print("\n");
    print("Creates or updates archive directory tree,\n");
    print("creates config files for daemons and engines\n");
    print("based on $config_name.\n");
    print("\n");
    print("Options:\n");
    print(" -h          : help\n");
    print(" -c <config> : Use given config file instead of $config_name\n");
    print(" -s <system> : Handle only the given system daemon, not all daemons.\n");
    print("               (Regular expression for daemon name)\n");
    print(" -d          : debug\n");
}

# Configuration info filled by parse_config_file
my ($config);

sub check_filename($)
{
    my ($filename) = @ARG;
    my ($response);
    if (-f $filename)
    {
	printf("'$filename' already exists.\n");
	printf("[O]verwrite, use [n]ew '$filename.new', or Skip (default) ? >");
	$response = <>;
	return $filename if ($response =~ '\A[oO](ve?)?');
	return "$filename.new" if ($response =~ '\A[nE](ew?)?');
	return "";
    }
    return $filename;
}

# Create scripts for engine
sub create_engine_files($$)
{
    my ($d_dir, $e_dir) = @ARG;
    my ($filename, $daemonfile, $daemon, $engine, $old_fd);

    # Engine Stop Script
    $filename = "$d_dir/$e_dir/stop-engine.sh";
    open OUT, ">$filename" or die "Cannot open $filename\n";
    $old_fd = select OUT;
    printf("#!/bin/sh\n");
    printf("#\n");
    printf("# Stop the %s engine (%s)\n",
           $e_dir,
           $config->{daemon}{$d_dir}{engine}{$e_dir}{desc});
    printf("\n");
    printf("lynx -dump http://%s:%d/stop\n",   
           $hostname,
           $config->{daemon}{$d_dir}{engine}{$e_dir}{port});
    select $old_fd;
    close OUT;
    chmod(0755, $filename);

    # ASCIIConfig/convert_example.sh
    $filename = "$d_dir/$e_dir/ASCIIConfig/convert_example.sh";
    open OUT, ">$filename" or die "Cannot open $filename\n";
    $old_fd = select OUT;
    printf("#!/bin/sh\n");
    printf("#\n");
    printf("# Example script for creating an XML engine config.\n");
    printf("#\n");
    printf("# THIS ONE WILL BE OVERWRITTEN!\n");
    printf("# Copy to e.g. 'convert.sh' and\n");
    printf("# modify the copy to suit your needs.\n");
    printf("#\n");
    printf("\n");
    printf("echo \"Creating 'example' channel list...\"\n");
    printf("echo \"# Example channel list\"  >example\n");
    printf("echo \"\"                        >>example\n");
    printf("echo \"fred 5\"                  >>example\n");
    printf("echo \"janet 1 Monitor\"         >>example\n");
    printf("\n");
    printf("REQ=\"\"\n");
    printf("REQ=\"\$REQ example\"\n");
    printf("\n");
    printf("echo \"Converting to engine config file...\"\n");
    printf("ConvertEngineConfig.pl -v -d %s -o ../%s-group.xml \$REQ\n",
	   $engine_dtd, $e_dir);
    print("\n");
    select $old_fd;
    close OUT;
    chmod(0755, $filename);
}

# Create scripts for daemon
sub create_daemon_files($)
{
    my ($d_dir) = @ARG;
    my ($filename, $daemonfile, $e_dir, $old_fd);
    
    $daemonfile = $d_dir . "-daemon.xml";

    # Daemon config file
    $filename = "$d_dir/$daemonfile";
    if (is_localhost($config->{daemon}{$d_dir}{run}))
    {
        open OUT, ">$filename" or die "Cannot open $filename\n";
        $old_fd = select OUT;
        printf("<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\n");
        printf("<!DOCTYPE daemon SYSTEM \"%s\">\n",
	       $daemon_dtd);
        printf("\n");
        printf("<!-- \n");
        printf("  NOTE:\n");
        printf("  This file will be OVERWRITTEN\n");
        printf("  with information from $config_name.\n");
        printf("\n");
        printf("  Change $config_name and re-run\n");
        printf("  update_archive_tree.pl\n");
        printf("  -->\n");
        printf("\n");
        printf("<daemon>\n");
        printf("    <port>$config->{daemon}{$d_dir}{port}</port>\n");
        if (exists($config->{mailbox}))
        {
            printf("    <mailbox>$config->{mailbox}</mailbox>\n");
        }
        foreach $e_dir ( sort keys %{ $config->{daemon}{$d_dir}{engine} } )
        {
	    if (! is_localhost($config->{daemon}{$d_dir}{engine}{$e_dir}{run}))
	    {
	        printf("    <!-- not running\n");
	    }
	    printf("    <engine directory='$e_dir'>\n");
	    printf("        <desc>%s</desc>\n",
	           $config->{daemon}{$d_dir}{engine}{$e_dir}{desc});
	    printf("        <port>%d</port>\n",
	           $config->{daemon}{$d_dir}{engine}{$e_dir}{port}); 
	    printf("        <config>%s-group.xml</config>\n",
	           $e_dir);
	    if (length($config->{daemon}{$d_dir}{engine}{$e_dir}{restart}{type}) > 0)
	    {
	        printf("        <restart type='%s'>%s</restart>\n",
		       $config->{daemon}{$d_dir}{engine}{$e_dir}{restart}{type},
		       $config->{daemon}{$d_dir}{engine}{$e_dir}{restart}{content});
	    }
            if (exists($config->{daemon}{$d_dir}{engine}{$e_dir}{dataserver}{host}))
            {
	        printf("        <dataserver><host>%s</host></dataserver>\n",
                       $config->{daemon}{$d_dir}{engine}{$e_dir}{dataserver}{host});
            }
	    printf("    </engine>\n");
	    if (! is_localhost($config->{daemon}{$d_dir}{engine}{$e_dir}{run}))
	    {
	        printf("    :: not running -->\n");
	    }
        }
        printf("</daemon>\n");
        select $old_fd;
        close OUT;
    }
    else
    {
	unlink($filename);
    }

    # Daemon Start Script
    $filename = "$d_dir/run-daemon.sh";
    if (is_localhost($config->{daemon}{$d_dir}{run}))
    {
        open OUT, ">$filename" or die "Cannot open $filename\n";
        $old_fd = select OUT;
        printf("#!/bin/sh\n");
        printf("#\n");
	printf("# Launch the %s daemon (%s)\n",
	       $d_dir,
	       $config->{daemon}{$d_dir}{desc});
	printf("\n");
	printf("cd %s/%s\n",
	       $config->{root}, $d_dir);
	printf("perl %s/scripts/ArchiveDaemon.pl -f %s/%s/%s\n",   
               $config->{root}, $config->{root}, $d_dir, $daemonfile);
        select $old_fd;
        close OUT;
        chmod(0755, $filename);
    }
    else
    {
	unlink($filename);
    }

    # Daemon Stop Script
    $filename = "$d_dir/stop-daemon.sh";
    if (is_localhost($config->{daemon}{$d_dir}{run}))
    {
        open OUT, ">$filename" or die "Cannot open $filename\n";
        $old_fd = select OUT;
        printf("#!/bin/sh\n");
        printf("#\n");
        printf("# Stop the %s daemon (%s)\n",
	       $d_dir,
	       $config->{daemon}{$d_dir}{desc});
        printf("\n");
        printf("lynx -dump http://%s:%d/stop\n",
	       $hostname,
	       $config->{daemon}{$d_dir}{port});
        select $old_fd;
        close OUT;
        chmod(0755, $filename);
    }
    else
    {
        unlink($filename);
    }

    # Daemon View Script
    $filename = "$d_dir/view-daemon.sh";
    if (is_localhost($config->{daemon}{$d_dir}{run}))
    {
        open OUT, ">$filename" or die "Cannot open $filename\n";
        $old_fd = select OUT;
        printf("#!/bin/sh\n");
        printf("#\n");
        printf("\n");
        printf("lynx http://%s:%d\n",
	       $hostname,
	       $config->{daemon}{$d_dir}{port});
        select $old_fd;
        close OUT;
        chmod(0755, $filename);
    }
    else
    {
        unlink($filename);
    }
}

# Create the daemon and engine directories
sub create_stuff()
{
    my ($d_dir, $e_dir);
    foreach $d_dir ( sort keys %{ $config->{daemon} } )
    {
	# Skip daemons/systems that don't match the supplied reg.ex.
	next if (length($opt_s) > 0 and not $d_dir =~ $opt_s);

	print("Daemon $d_dir\n");
	# Daemon Directory
        if (is_localhost($config->{daemon}{$d_dir}{run}))
        {
	    mkpath($d_dir, 1);
	    create_daemon_files($d_dir);
        }
        foreach $e_dir ( sort keys %{ $config->{daemon}{$d_dir}{engine} } )
	{
	    # Engine and ASCII-config dir
            if (is_localhost($config->{daemon}{$d_dir}{engine}{$e_dir}{run}))
            {
	        print(" - Engine $e_dir\n");
	        mkpath("$d_dir/$e_dir", 1);    
	        mkpath("$d_dir/$e_dir/ASCIIConfig", 1);    
	        create_engine_files($d_dir, $e_dir);
            }
	}
    }
}

# Look for dirs that are not described
sub check_dirs()
{
    my ($d_dir, $sub, $e_dir);

    foreach $d_dir ( <*> )
    {
        next unless -d $d_dir;
        next if ($d_dir eq "scripts");
        next if (exists($config->{mailbox}) && ($config->{mailbox} =~ /$d_dir/));
        if (not exists $config->{daemon}{$d_dir})
        {
            print("Directory '$d_dir' is not described in the configuration\n");
            next;
        }
        foreach $sub ( <$d_dir/*> )
        {
            next unless -d $sub;
            my ($x, $e_dir) = split('/', $sub);
            if (not exists $config->{daemon}{$d_dir}{engine}{$e_dir})
            {
                print("Directory '$d_dir/$e_dir' is not described in the configuration\n");
                next;
            }
        }
    }
}

# The main code ==============================================

# Parse command-line options
if (!getopts("dhc:s:") ||  $opt_h)
{
    usage();
    exit(0);
}
$config_name = $opt_c if (length($opt_c) > 0);
$config = parse_config_file($config_name, $opt_d);

my ($root)  = $config->{root};
die "Should run in '$root', not '$path'\n" unless ($root eq $path);
$index_dtd  = "$root/indexconfig.dtd";
$daemon_dtd = "$root/ArchiveDaemon.dtd";
$engine_dtd = "$root/engineconfig.dtd";

create_stuff();

check_dirs() if (length($opt_s) <= 0);


