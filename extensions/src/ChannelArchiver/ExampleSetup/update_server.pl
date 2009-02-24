#!/usr/bin/perl
#
# Based on Greg Lawson's relocate_arch_data.pl
#
# Reads the mailbox dir, copies new data,
# and re-indexes.
#
# For the copy to work via scp,
# key pairs need to be configured.
# Assume this is the data server computer that
# needs to use scp to pull data off archive computers.
#
# Assume the script runs as user 'xfer'
# and needs to pull data from 'arch1':
#
# Once only:
# ssh-keygen -t dsa
#
# Copy public key over:
# scp ~/.ssh/id_dsa.pub xfer@arch1:~/.ssh/xx
#
# Configure public key on arch1:
# ssh arch1
# cd .ssh/
# cat xx >>authorized_keys 
# rm xx
# chmod 600 authorized_keys 
#
# From now on, you should be able to go to 'arch1'
# without being queried for a pw.

BEGIN
{
    push(@INC, 'scripts' );
    push(@INC, '/arch/scripts' );
}


use English;
use strict;
use vars qw($opt_d $opt_h $opt_c $opt_n $opt_t $opt_5);
use Cwd;
use File::Path;
use Getopt::Std;
use Data::Dumper;
use Sys::Hostname;
use archiveconfig;

# Very bad, not sure how to better solve this:
# At the SNS, the archive computer might consider itself 'ics-srv-archive1',
# and we are to copy data from 'ics-srv-archive1:/arch/....' to the server.
# But from the data server machine, it's accessible as 'arch1'.
# So we need to translate some addresses:
my (%host_hacks) =
(
    'ics-srv-archive1' => 'arch1',
    'ics-srv-archive2' => 'arch2',
    'ics-srv-archive3' => 'arch3',
);

# Globals, Defaults
my ($config_name) = "archiveconfig.xml";
my ($path) = cwd();

sub usage()
{
    print("USAGE: update_server [options]\n");
    print("\n");
    print("Reads $config_name, checks <mailbox> directory\n");
    print("for new data.\n");
    print("Copies data meant for this server,\n");
    print("then performs an index update.\n");
    print("\n");
    print("Options:\n");
    print(" -h          : help\n");
    print(" -c <config> : Use given config file instead of $config_name\n");
    print(" -d          : debug\n");
    print(" -n          : nop, don't run any commands, just print them\n");
    print(" -t          : only execute the file transfers, skip the index_update\n");
    print(" -5          : no 'md5sum' checksum\n");
}

# Configuration info filled by parse_config_file
my ($config);

sub check_mailbox()
{
    my ($updates) = 0;
    my ($entry);
    return 0 unless exists($config->{mailbox});
    chdir($config->{mailbox});
    # An update run can take some time.
    # We don't want multiple 'update' scripts handling the same mailbox entry.
    # So mailbox files are moved into 'active' when being read,
    # then into 'done' when done.
    # Some might stay in active because there's a problem.
    # That needs to be handled manually.
    mkpath("active");
    mkpath("done");
    # Loop over files in mailbox directory
    my (@files) = <*>;
    my ($num_files) = $#files - 1;
    my ($current) = 0;
    foreach $entry ( @files )
    {
        # Skip directories like 'active' and 'done'
        next unless (-f $entry);
        ++$current;
	print("** $entry ($current / $num_files):\n");
        my ($handled) = 0;
        rename($entry, "active/$entry") or die "Cannot move $entry to 'active' subdir: $!\n";
	open(MB, "active/$entry") or die "Cannot open 'active/$entry'\n";
	while (<MB>)
        {   # Each file should contain 'new ... ' or 'copy ...' info.
            chomp;
            print("$_\n") if ($opt_d);
            my ($info, $src, $dst) = split(/\s+/);
            my ($src_host, $src_dir) = split(/:/, $src);
            my ($dst_host, $dst_dir) = split(/:/, $dst);
            if ($info eq "new" and defined($src_host) and is_localhost($src_host))
            {   # Data already here, we need to update
                ++$updates;
                $handled = 1;
            }
            elsif ($info eq "copy" and defined($dst_host) and is_localhost($dst_host))
            {   # Copy data here, then update.
                # Careful with 'scp -r':
                # It will create the target dir, e.g. 2006/03_01.
                # But, if the target dir already exists,
                # it will create 2006/03_01/03_01!!
                # -> don't use -r
		$src_host = $host_hacks{$src_host} if (defined($host_hacks{$src_host}));

                # Compute remote md5sum
                my ($cmd);
                $cmd = "mkdir -p $dst_dir";
                print("$cmd\n");
                unless ($opt_n)
                {
                    die "Error while running command\n" if (system($cmd));
                }
                unless ($opt_5)
                {
                    $cmd = "ssh $src_host 'md5sum -b $src_dir/*' >$dst_dir/md5";
                    print("$cmd\n");
                    unless ($opt_n)
                    {
                        die "Error while running command\n" if (system($cmd));
                    }
                }
           
                # Copy data. Works only with mkdir -p from previous call!!
                $cmd = "scp $src_host:$src_dir/* $dst_dir";
                print("$cmd\n");
                unless ($opt_n)
                {
                    die "Error while running command\n" if (system($cmd));
                }

                # Compare md5sum output...
                unless ($opt_5)
                {
                    $cmd = "md5sum --check $dst_dir/md5";
                    print("$cmd\n");
                    unless ($opt_n)
                    {
                        die "Error while running command\n" if (system($cmd));
                    }
                }

                ++$updates;
                $handled = 1;
            }
        }
        close(MB);
        # rename
        rename("active/$entry", "done/$entry") if ($handled);
    } 
    return $updates;
} 

# The main code ==============================================

# Parse command-line options
if (!getopts("dhc:nt5") ||  $opt_h)
{
    usage();
    exit(0);
}
$config_name = $opt_c if (length($opt_c) > 0);
$config = parse_config_file($config_name, $opt_d);

die "Has to run in $config->{root}\n" unless ($path eq $config->{root});

# NOP includes: no index update
$opt_t = 1 if ($opt_n);

my ($updates) = check_mailbox();
chdir($path);
if ($updates)
{
    my ($cmd) = "perl scripts/update_indices.pl";
    print("$cmd\n");
    system($cmd) unless ($opt_t);
}


