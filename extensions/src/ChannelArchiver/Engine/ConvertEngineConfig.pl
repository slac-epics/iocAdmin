#!/usr/bin/perl
# ConvertEngineConfig.pl
#
# kasemir@lanl.gov

use English;
#use strict;
use vars qw($opt_v $opt_d $opt_o);
use Getopt::Std;
use File::Basename;

sub usage()
{
    print("USAGE: ConvertEngineConfig [options] ASCII-config { ASCII-config }\n");
    print("\n");
    print("Options:\n");
    print("  -v             :  Verbose\n");
    print("  -d <DTD>       :  Specify DTD\n");
    print("  -o <filename>  :  Set output file name\n");
    print("\n");
    print("This tool reads an ArchiveEngine's ASCII configuration\n");
    print("file and converts it into the XML config file.\n");
    print("\n");
    print("One can use either one ASCII config file (with !group entries)\n");
    print("or supply a list of ASCII files, where each one will then\n");
    print("define a group.\n");
    exit(-1);
}

if (!getopts('vd:o:')  ||  $#ARGV < 0)
{
    usage();
}
my ($outfile) = "";
if (length($opt_o) > 0)
{
    $outfile = $opt_o;
    print("Creating output file '$outfile'\n") if ($opt_v);
}

unless (length($opt_d) > 0)
{
    $opt_d = "engineconfig.dtd";
    printf STDERR ("\nUsing '$opt_d' as the DTD\n");
    printf STDERR ("You should use the '-d DTD' option\n");
    printf STDERR ("to provide the path to your DTD.\n\n");
}
my (%params, %groups, $filename, $directory, $file, $filesize_warning);
# Defaults for the global options
%params = 
(
 write_period => 30,
 get_threshold => 20,
 file_size => 30,
 ignored_future => 1.0,
 buffer_reserve => 3,
 max_repeat_count => 120
);
$filesize_warning = 0;

foreach $filename ( @ARGV )
{
    print("- group '$filename'\n") if ($opt_v);
    $directory = dirname($filename);
    $file = basename($filename);
    parse($file,'fh00');
}
dump_xml();
print("Done.\n") if ($opt_v);

# parse(<group file name>, file handle to use)
# creates something like this:
# %groups =
# (
#  "excas" =>
#  {
#      "fred" => { period => 5, scan => 1 },
#      "janet" => { period => 1, monitor => 1, disable => 1 },
#  },
# );
#
# file handle is used to that parse() can recurse.
# 'recurse': See 'recurse'
sub parse($$)
{
    my ($group,$fh) = @ARG;
    my ($channel, $period, $options, $opt, $monitor, $disable, $line);
    $fh++;
    if (not open $fh, "$group")
    {
        open $fh, "$directory/$group"
            or die "Cannot open $group nor $directory/$group\n";
    }
    $group = basename($group);
    while (<$fh>)
    {
        chomp;
        next if (m'\A#'); # Skip comments
        next if (m'\A\s*\Z'); # Skip empty lines
        $line = $_;
        if (m'![Dd]isconnect\s*')
        {   # Parameter "!disconnect" (you knew that)
            $params{disconnect} = 1;
        }
        elsif (m'!(\S+)\s+(\S+)')
        {   # Parameter "!<text> <text>" ?
            if ($1 eq "group")
            {
                parse($2,$fh);
            }
            elsif (length($params{$1}) > 0)
            {   # Check if it's a valid param (is there a default value?)
                $params{$1} = $2;
                if ($1 eq "file_size")
                {
                    $params{$1} = $2/24 * 10;
                    if (not $filesize_warning)
                    {
                        printf("%s, line %d:\n", $group, $NR);
                        printf("\tThe 'file_size' parameter used to be in hours,\n");
                        printf("\tbut has been changed to MB.\n");
                        printf("\tThe automated conversion is arbitrary, assuming a\n");
                        printf("\tdata file size of about 10 MB per day (24h).\n");
                        $filesize_warning = 1;
                    }
                }
            }
            else
            {
                printf("%s, line %d: Unknown parameter/value '%s' - '%s'\n",
                       $group, $NR, $1, $2);
                exit(-2);
            }
        }
        elsif (m'\A\s*(\S+)\s+([0-9\.]+)(\s+)?(.+)?\Z')
        {   # <channel> <period> [Monitor|Disable]*
            $channel = $1;
            $period = $2;
            $opt_delim = $3;
            $options = $4;
            $monitor = $disable = 0;
            if (not defined($period)  or  $period <= 0)
            {
                printf("%s, line %d: Invalid scan period\n",
                       $group, $NR);
                exit(-3);
            }
            if (length($options) > 0)
            {
                if (length($opt_delim) <= 0)
                {
                    printf("%s, line %d: Missing space between period and options\n",
                           $group, $NR);
                    exit(-4);
                }
                foreach $opt ( split /\s+/, $options )
                {
                    if ($opt =~ m'\A[Mm]onitor\Z')
                    {
                        $monitor = 1;
                    }
                    elsif ($opt =~ m'\A[Dd]isable\Z')
                    {
                        $disable = 1;
                    }
                    elsif (length($opt) > 0)
                    {
                        printf("%s, line %d: Invalid option '%s'\n",
                               $group, $NR, $opt);
                        exit(-5);
                    }
                }
            }
            $groups{$group}{$channel}{period} = $period;
            if ($monitor)
            {
                $groups{$group}{$channel}{monitor} = $monitor;
            }
            else
            {
                $groups{$group}{$channel}{scan} = 1;
            }
            $groups{$group}{$channel}{disable} = $disable;
        }
        else
        {
            printf("%s, line %d:\n '%s' is neither comment, " .
		   "option nor channel definition\n",
                   $group, $NR, $line);
            exit(-6);
        }
    }
    close($fh);
}

sub dump_xml()
{
    my ($parm, $group, $channel, $ofh);
    if (length($outfile) > 0)
    {
        open OUT, ">$outfile" or die "Cannot create $outfile\n";
        $ofh = select(OUT);
    }
    printf("<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\n");
    printf("<!DOCTYPE engineconfig SYSTEM \"$opt_d\">\n");
    printf("<engineconfig>\n");
    foreach $parm ( sort keys %params )
    {
        if ($parm eq "disconnect")
        {
            printf("\t<disconnect/>\n");
        }
        else
        {
            printf("\t<$parm>$params{$parm}</$parm>\n");
        }
    }
    
    foreach $group ( sort keys %groups )
    {
        printf("\t<group>\n");
        printf("\t\t<name>$group</name>\n");
        foreach $channel ( sort keys %{$groups{$group}} )
        {
            printf("\t\t<channel><name>$channel</name>");
            printf("<period>$groups{$group}{$channel}{period}</period>");

            if ($groups{$group}{$channel}{monitor}  and
		$groups{$group}{$channel}{scan})
	    {
		die "Channel '$channel' is defined as both monitored and scanned\n";
	    }
            printf("<monitor/>") if ($groups{$group}{$channel}{monitor});
            printf("<scan/>")    if ($groups{$group}{$channel}{scan});

            printf("<disable/>") if ($groups{$group}{$channel}{disable});
            printf("</channel>\n");
        }
        printf("\t</group>\n");
    }
    printf("</engineconfig>\n");
    if (length($outfile) > 0)
    {
        select($ofh);
        close(OUT);
    }
}
