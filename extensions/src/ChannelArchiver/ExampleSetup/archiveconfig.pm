package archiveconfig;
# Perl module to help with the archiveconfig.xml file.
#
# Reads the data as XML::Simple reads the XML file:
# $config->{daemon}{$d_dir}{port}
# $config->{daemon}{$d_dir}{desc}
# $config->{daemon}{$d_dir}{engine}{$e_dir}{port}
# $config->{daemon}{$d_dir}{engine}{$e_dir}{desc}
# $config->{daemon}{$d_dir}{engine}{$e_dir}{restart}{type}
# $config->{daemon}{$d_dir}{engine}{$e_dir}{restart}{content}
#
# and adds some info in update_status:
# $config->{daemon}{$d_dir}{status}
# $config->{daemon}{$d_dir}{engine}{$e_dir}{status}

require Exporter;
@ISA = qw(Exporter);
@EXPORT = qw(time_as_text parse_config_file is_localhost read_URL update_status);

use English;
use strict;
use Socket;
use IO::Handle;
use Sys::Hostname;
use Data::Dumper;
# Linux and MacOSX seem to include this one per default:
use LWP::Simple;
use XML::Simple;

# Timeout used when reading a HTTP client or ArchiveEngine.
my ($read_timeout) = 30;
my ($localhost) = hostname();

sub time_as_text($)
{
    my ($seconds) = @ARG;
    my ($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst)
        = localtime($seconds);
    return sprintf("%04d/%02d/%02d %02d:%02d:%02d",
           1900+$year, 1+$mon, $mday, $hour, $min, $sec);
}

# Parse the old-style tab separated file format.
#
# The file is supposed to be a TAB-delimited table of the following
# columns:
#
# Type, Name, Port, Description, Time, Frequency
# ----------------------------------------------
# Type:        'DAEMON' or 'ENGINE'
# Name:        Name of daemon or engine subdir
# Port:        TCP port used by daemon or engine
# Description: Any text. Defaults to name it left empty.
# Time:        Time in HH:MM format for the engine restarts
# Frequency:   'daily' or 'weekly' or ...
#
# Returns $config
sub parse_tabbed_file($$)
{
    my ($filename, $opt_d) = @ARG;
    my ($config);
    my ($in);
    my ($type, $name, $d_dir, $e_dir, $port, $desc, $time, $restart);
    my ($di, $ei); # Index of current daemon and engine

    print("Parsing '$filename' as tabbed file.\n") if ($opt_d);
    open($in, $filename) or die "Cannot open '$filename'\n";
    print("Reading $filename\n") if ($opt_d);
    while (<$in>)
    {
        chomp;                          # Chop CR/LF
        next if ($ARG =~ '\A#');        # Skip comments
        next if ($ARG =~ '\A[ \t]*\Z'); # ... and empty lines
        ($type,$name,$port,$desc,$restart,$time) = split(/\t/, $ARG); # Get columns
        if ($type eq "DAEMON")
        {
            $d_dir = $name;
            $desc = "$d_dir Daemon" unless (length($desc) > 0); # Desc default.
            print("$NR: Daemon '$d_dir', Port $port, Desc '$desc'\n") if ($opt_d);
            $config->{daemon}{$d_dir}{desc} = $desc;
            $config->{daemon}{$d_dir}{port} = $port;
        }
        elsif ($type eq "ENGINE")
        {
            $e_dir = $name;
            $desc = "$e_dir Engine" unless (length($desc) > 0); # Desc default.
            print("$NR: Engine '$e_dir', Port $port, Desc '$desc', Time '$time', Restart '$restart'\n") if ($opt_d);
            $config->{daemon}{$d_dir}{engine}{$e_dir}{desc} = $desc;
            $config->{daemon}{$d_dir}{engine}{$e_dir}{port} = $port;
            $config->{daemon}{$d_dir}{engine}{$e_dir}{restart}{type} = $restart;
            $config->{daemon}{$d_dir}{engine}{$e_dir}{restart}{content} = $time;
        }
        else
        {
            die("File '$filename', line $NR: Cannot handle type '$type'\n");
        }
    }
    close $in;
    return $config;
}

# Parse XML or old-style tab separated file format.
#
# Returns $config
sub parse_config_file($$)
{
    my ($filename, $opt_d) = @ARG;
    my ($config);

    print("Parsing '$filename'\n") if ($opt_d);
    if ($filename =~ m".csv\Z")
    {
        $config = parse_tabbed_file($filename, $opt_d)
    }
    else
    {
        $config = XMLin($filename, ForceArray => [ 'daemon', 'engine' ], KeyAttr => "directory");
    }
    if ($opt_d)
    {
        print "Config as parsed from XML file:\n";
        print Dumper($config);
        print "-------------------------------------\n";
    }
    check_config($config, $opt_d);
    if (0)
    {
        print "-------------------------------------\n";
        print "Config after adding defaults:\n";
        print Dumper($config);
        print "-------------------------------------\n";
    }

    return $config;
}

# Check configuration, add defaults for missing elements
sub check_config($$)
{
    my ($config, $opt_d) = @ARG;
    my ($d_dir, $e_dir);
    my (%ports, %keys);
    print "ERROR: The mailbox directory doesn't exist\n" unless (-d  $config->{mailbox});

    # Loop over daemons
    foreach $d_dir ( sort keys %{ $config->{daemon} } )
    {
        print "Daemon '$d_dir'\n" if ($opt_d);
        my ($daemon) = $config->{daemon}{$d_dir};

        # Where daemon runs
        unless (exists $daemon->{run})
        {
            $daemon->{run} = "false";
            print "    (run entry defaults to 'false')\n" if ($opt_d);
        }
        if (is_localhost($daemon->{run}))
        {
            print "    run='$daemon->{run}' (runs on this host)\n" if ($opt_d);
        }
        else
        {
            print "    run='$daemon->{run}' (not this host)\n" if ($opt_d);
        }

        # Description
        unless (exists $daemon->{desc})
        {
            $daemon->{desc} = "$d_dir Daemon";
            print "    (description defaults to directory name)\n" if ($opt_d);
        }
        print "    description='$daemon->{desc}'\n" if ($opt_d);

        # Port
        if (exists $daemon->{port})
        {
            print "    port='$daemon->{port}'\n" if ($opt_d);
            print "ERROR: Port $daemon->{port} already used by $ports{$daemon->{port}}\n"
                if (exists $ports{$daemon->{port}});
            $ports{$daemon->{port}} = "Daemon $d_dir";
        }
        elsif ($daemon->{run} ne "false")
        {
            print "ERROR: Since daemon runs on host '$daemon->{run}', a port must be specified\n";
        }

        # Data server
        if (exists($daemon->{dataserver}))
        {
            print "    Data Server for combined engine index:\n" if ($opt_d);
            # Host entry
            unless (exists $daemon->{dataserver}{host})
            {
                $daemon->{dataserver}{host} = "false";
                print "        (host entry defaults to 'false')\n" if ($opt_d);
            }
            my ($is_local) = is_localhost($daemon->{dataserver}{host});
            if ($is_local)
            {
                print "        host='$daemon->{dataserver}{host}' (runs on this host)\n" if ($opt_d);
            }
            else
            {
                print "        host='$daemon->{dataserver}{host}' (not this host)\n" if ($opt_d);
            }

            # Accidental current_index?
            print "ERROR: The current_index entry only applies to the engine data server,"
                  . " not the daemon data server\n"
                if (exists $daemon->{dataserver}{current_index});

            # Decide index: list, binary, ...
            if (exists  $daemon->{dataserver}{index})
            {   # daemon indices require a key
                print "ERROR: Missing key for index '$daemon->{dataserver}{index}{content}'\n"
                    unless (exists $daemon->{dataserver}{index}{key});

                # Check for duplicate keys
                my ($key) = $daemon->{dataserver}{index}{key};
                print "ERROR: Daemon key $key already used by $keys{$key}\n"
                    if (exists $keys{$key});
                $keys{$key} = "Daemon data server '$daemon->{dataserver}{index}{content}'";

                if ($daemon->{dataserver}{index}{type} eq "serve")
                {   # 'serve' type index requires file name and key
                    print "ERROR: Missing file name for index '$daemon->{dataserver}{index}{content}'\n"
                        unless (exists $daemon->{dataserver}{index}{file});
                    my ($file) = "$config->{root}/$d_dir/$daemon->{dataserver}{index}{file}";
                    if ($is_local and not -r $file)
                    {
                        print "WARNING: File '$file' is not accessible'\n";
                    }
                    printf "        file '%s' served as '%s', key '%s'\n",
                        $file, $daemon->{dataserver}{index}{content}, $daemon->{dataserver}{index}{key} if ($opt_d);
                }
                elsif ($daemon->{dataserver}{index}{type} eq "binary")
                {   # create and maybe serve binary index
                    my ($file) = "$config->{root}/$d_dir/master_index";
                    printf "        creating indexconfig.xml and binary index '%s',", $file if ($opt_d);
                    printf " served as '%s', key '%s'\n",
                        $daemon->{dataserver}{index}{content}, $daemon->{dataserver}{index}{key} if ($opt_d);
                }
                else
                {   # list index
                    my ($file) = "$config->{root}/$d_dir/indexconfig.xml";
                    printf "        creating list index '%s',", $file if ($opt_d);
                    printf " served as '%s', key '%s'\n",
                        $daemon->{dataserver}{index}{content}, $daemon->{dataserver}{index}{key} if ($opt_d);
                }
            }
            else
            {
                print "ERROR: No 'index' entry, so nothing to serve!\n"
            }
        }

        # Loop over engines for this daemon
        foreach $e_dir ( sort keys %{ $daemon->{engine} } )
        {
            print "    Engine '$d_dir/$e_dir'\n" if ($opt_d);
            my ($engine) = $daemon->{engine}{$e_dir};
            # Where engine runs
            unless (exists $engine->{run})
            {
                $engine->{run} = "false";
                print "        (run entry defaults to 'false')\n" if ($opt_d);
            }
            if (is_localhost($engine->{run}))
            {
                print "        run='$engine->{run}' (runs on this host)\n" if ($opt_d);
            }
            else
            {
                print "        run='$engine->{run}' (not this host)\n" if ($opt_d);
            }

            # Description
            unless (exists $engine->{desc})
            {
                $engine->{desc} = "$e_dir";
                print "        (description defaults to directory name)\n" if ($opt_d);
            }
            print "        description='$engine->{desc}'\n" if ($opt_d);

            # Port
            if (exists $engine->{port})
            {
                print "        port='$engine->{port}'\n" if ($opt_d);

                print "ERROR: Port $engine->{port} already used by $ports{$engine->{port}}\n"
                    if (exists $ports{$engine->{port}});
                $ports{$engine->{port}} = "Engine $e_dir";

            }
            elsif ($engine->{run} ne "false")
            {
                print "ERROR: Since engine runs on host '$engine->{run}', a port must be specified\n";
            }

            # Restart
            if (exists $engine->{restart}{type})
            {
                print "        restart $engine->{restart}{type}, $engine->{restart}{content}\n" if ($opt_d);
            }
            else
            {
                $engine->{restart}{type} = "never";
                $engine->{restart}{content} = "";
                print "        no restart\n" if ($opt_d);
            }

            # Data server
            if (exists($engine->{dataserver}))
            {
                print "        Data Server for this engine:\n" if ($opt_d);
                # Host
                unless (exists $engine->{dataserver}{host})
                {
                    $engine->{dataserver}{host} = "false";
                    print "            (host entry defaults to 'false')\n" if ($opt_d);
                }
                my ($is_local) = is_localhost($engine->{dataserver}{host});
                if ($is_local)
                {
                    print "            host='$engine->{dataserver}{host}' (runs on this host)\n" if ($opt_d);
                }
                else
                {
                    print "            host='$engine->{dataserver}{host}' (not this host)\n" if ($opt_d);
                }
                my ($anything) = 0;
                # current_index
                if (exists $engine->{dataserver}{current_index})
                {
                    printf "            current_index='%s', key '%s'\n",
                        $engine->{dataserver}{current_index}{content},
                        $engine->{dataserver}{current_index}{key} if ($opt_d);
                    my ($file) = "$config->{root}/$d_dir/$e_dir/current_index";
                    if ($is_local and not -r $file)
                    {
                        print "WARNING: $file doesn't exist!\n";
                    }

                    # Check for duplicate keys
                    my ($key) = $engine->{dataserver}{current_index}{key};
                    print "ERROR: Engine key $key already used by $keys{$key}\n"
                        if (exists $keys{$key});
                    $keys{$key} = "Engine data server '$engine->{dataserver}{current_index}{content}'";

                    $anything = 1;
                }
                # other index entries
                if (exists $engine->{dataserver}{index})
                {
                    if ($engine->{dataserver}{index}{type} eq "list")
                    {
                        print "ERROR: 'list' index is not supported for engines\n";
                    }
                    elsif ($engine->{dataserver}{index}{type} eq "serve")
                    {   # 'serve' type index requires file name and key
                        print "ERROR: Missing key for index '$engine->{dataserver}{index}{content}'\n"
                            unless (exists $engine->{dataserver}{index}{key});
                        print "ERROR: Missing file name for index '$engine->{dataserver}{index}{content}'\n"
                            unless (exists $engine->{dataserver}{index}{file});
                        my ($file) = "$config->{root}/$d_dir/$e_dir/$engine->{dataserver}{index}{file}";
                        if ($is_local and not -r $file)
                        {
                            print "WARNING: File '$file' is not accessible'\n";
                        }
                        printf "            file '%s' served as '%s', key '%s'\n",
                            $file, $engine->{dataserver}{index}{content}, $engine->{dataserver}{index}{key} if ($opt_d);
                        $anything = 1;
                    }
                    elsif ($engine->{dataserver}{index}{type} eq "binary")
                    {   # create and maybe serve binary index
                        my ($file) = "$config->{root}/$d_dir/$e_dir/master_index";
                        printf "            creating indexconfig.xml and binary index '%s'\n", $file if ($opt_d);
                        if (exists $engine->{dataserver}{index}{key})
                        {
                            printf "            served as '%s', key '%s'\n",
                                $engine->{dataserver}{index}{content}, $engine->{dataserver}{index}{key} if ($opt_d);
                        }
                        $anything = 1;
                    }

                    # Check for duplicate keys
                    if (exists $engine->{dataserver}{index}{key})
                    {
                        my ($key) = $engine->{dataserver}{index}{key};
                        print "ERROR: Engine key $key already used by $keys{$key}\n"
                            if (exists $keys{$key});
                        $keys{$key} = "Engine data server '$engine->{dataserver}{index}{content}'";
                    }
                }
                print "ERROR: No 'index' nor 'current_index' entry, so nothing to serve!\n"
                    unless $anything;
            }
        }
        print "\n" if ($opt_d);
    }
}

# For checking host-where-to-run or 'false'
# from <run> or <host> tags.
#
# Returns 1 if daemon or engine or ...
# should run on this computer because
# e.g. ics-srv-archive1 matched the <run>archive1</run>
# (so note: sub-matches are OK, too).
#
# And empty/missing <run></run> means: Run everywhere.
sub is_localhost($)
{
    my ($host) = @ARG;
    return 0 if ($host eq 'false');
    return 1 if (length($host) <= 0);
    return ($localhost =~ m/$host/); 
}

# Connects to HTTPD at host/port and reads a URL,
# returning the raw document.
sub read_URL($$$)
{
    my ($host, $port, $url) = @ARG;
    my ($content, @doc);
    $content = get("http://$host:$port/$url");
    @doc = split /[\r\n]+/, $content;
    return @doc;
}

# Input: $config, $debug
sub update_status($$)
{
    my ($config, $opt_d) = @ARG;
    my (@html, $line, $d_dir, $e_dir);
    foreach $d_dir ( keys %{ $config->{daemon} } )
    {
        # Assume the worst until we learn more
        foreach $e_dir ( keys %{ $config->{daemon}{$d_dir}{engine} } )
        {
            $config->{daemon}{$d_dir}{engine}{$e_dir}{status} = "unknown";
            $config->{daemon}{$d_dir}{engine}{$e_dir}{channels} = 0;
            $config->{daemon}{$d_dir}{engine}{$e_dir}{connected} = 0;
        }
        # Skip daemon if not supposed to run
        if ($config->{daemon}{$d_dir}{'run'} eq 'false')
        {
            $config->{daemon}{$d_dir}{status} = "not checked";
            next;
        }
        $config->{daemon}{$d_dir}{status} = "no response";
        print "Checking $d_dir daemon on port $config->{daemon}{$d_dir}{port}:\n" if ($opt_d);
        @html = read_URL($localhost, $config->{daemon}{$d_dir}{port}, "status");
        if ($opt_d)
        {
            print "Response from $d_dir:\n";
            foreach $line ( @html )
            {   print "    '$line'\n"; }
        }
        foreach $line ( @html )
        {
            if ($line =~ m"\AENGINE ([^|]*)\|([0-9]+)\|([^|]+)\|([0-9]+)\|([0-9]+)")
            {
                my ($port, $status, $connected, $channels) = ($2, $3, $4, $5);
                if ($opt_d)
                {
                    print("Engine: port $port, status $status, $connected/$channels connected\n");
                }
                $config->{daemon}{$d_dir}{status} = "running";
                foreach $e_dir ( keys %{ $config->{daemon}{$d_dir}{engine} } )
                {
                    if ($config->{daemon}{$d_dir}{engine}{$e_dir}{port} == $port)
                    {
                        $config->{daemon}{$d_dir}{engine}{$e_dir}{status} = $status;
                        $config->{daemon}{$d_dir}{engine}{$e_dir}{channels} = $channels;
                        $config->{daemon}{$d_dir}{engine}{$e_dir}{connected} = $connected;
                        last;
                    }
                }
            }
        }
    }
}


