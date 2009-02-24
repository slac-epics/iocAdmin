#!/usr/bin/perl
#
# Perl script for testing the Archiver's XML-RPC data server.
#
# Every once in a while, this script will report an error ala
#
# "not well-formed (invalid token) at line 21, column 2, byte 626
#  at /usr/lib/perl5/vendor_perl/...
#     ...5.8.0/i386-linux-thread-multi/XML/Parser.pm line 185"
#
# for the archiver.info call.
# A separate test with a shell script that directly sends the
# XML-RPC request to the ArchiveDataServer via stdin and compares
# the output with a known good output couldn't find any discrepancies.
#
# So the problem might be in the web server(unlikely) or
# the Frontier::Client, XML/Parter.pm, ...
#
# kasemir@lanl.gov

use English;
use Time::Local;
use Frontier::Client;
use Data::Dumper;
use strict;
use vars qw($opt_v $opt_u $opt_i $opt_a $opt_k $opt_l $opt_m $opt_h $opt_s $opt_e $opt_c);
use Getopt::Std;

my ($server, $result, $results, $key);

sub usage()
{
    print("USAGE: ArchiveDataClient.pl [options] { channel names }\n");
    print("\n");
    print("Options:\n");
    print("  -v         : Be verbose\n");
    print("  -u URL     : Set the URL of the DataServer\n");
    print("  -i         : Show server info\n");
    print("  -a         : List archives (name, key, path)\n");
    print("  -k key     : Specify archive key.\n");
    print("  -l         : List channels\n");
    print("  -m pattern : ... that match a patten\n");
    print("  -h how     : 'how' number; retrieval method\n");
    print("  -s time    : Start time MM/DD/YYYY HH:MM:SS.NNNNNNNNN\n");
    print("  -e time    : End time MM/DD/YYYY HH:MM:SS.NNNNNNNNN\n");
    print("  -c count   : Count\n");
    print("\n");
    exit(-1);
}

# Convert seconds & nanoseconds into string
sub time2string($$)
{
    my ($secs, $nano) = @ARG;
    my ($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) =
	localtime($secs);
    return sprintf("%02d/%02d/%04d %02d:%02d:%02d.%09ld",
		   $mon+1, $mday, $year + 1900, $hour, $min, $sec, $nano);
}

# Parse (seconds, nanoseconds) from string, couterpart to time2string
sub string2time($)
{
    my ($text) = @ARG;
    my ($secs, $nano);
    my ($mon, $mday, $year, $hour, $min, $sec);
    $hour = $min = $sec = $nano = 0;
    ($mon, $mday, $year, $hour, $min, $sec) = split '[/ :.]', $text;
    $secs=timelocal($sec, $min, $hour, $mday, $mon-1, $year-1900);

    printf("%s -> %d secs, %d nano\n", $text, $secs, $nano) if ($opt_v);
    return ( $secs, $nano );
}

# Convert (status, severity) into string
sub stat2string($$)
{
    my ($stat, $sevr, $ss) = @ARG;
 
    return "ARCH_DISABLED" if ($sevr == 0x0f08);
    return "ARCH_REPEAT $stat" if ($sevr == 0x0f10);
    return "ARCH_STOPPED" if ($sevr == 0x0f20);
    return "ARCH_DISCONNECT" if ($sevr == 0x0f40);
    return "ARCH_EST_REPEAT $stat" if ($sevr == 0x0f80);
    $sevr = $sevr & 0xF;

    $ss = "" if ($sevr == 1);
    $ss = "Severity $sevr";
    $ss = "MINOR" if ($sevr == 1);
    $ss = "MAJOR" if ($sevr == 2);
    $ss = "INVALID" if ($sevr == 3);

    return "" if ($stat == 0);
    return "READ/$ss" if ($stat == 1);
    return "WRITE/$ss" if ($stat == 2);
    return "HIHI/$ss" if ($stat == 3);
    return "HIGH/$ss" if ($stat == 4);
    return "LOLO/$ss" if ($stat == 5);
    return "LOW/$ss" if ($stat == 6);
    return "UDF/$ss" if ($stat == 17);
    return "Status $stat/$ss";
}

# local: dump meta info (prefix, meta-hash reference)
sub show_meta($$)
{
    my ($pfx, $meta, $i, $num) = @ARG;
    if ($meta->{type} == 1)
    {
	print($pfx, "Display : $meta->{disp_low} ... $meta->{disp_high}\n");
	print($pfx, "Alarms  : $meta->{alarm_low} ... $meta->{alarm_high}\n");
	print($pfx, "Warnings: $meta->{warn_low} ... $meta->{warn_high}\n");
	print($pfx, "Units   : '$meta->{units}', Precision: $meta->{prec}\n");
    }
    elsif ($meta->{type} == 0)
    { 
        $num = $#{$meta->{states}};
	for ($i=0; $i<=$num; ++$i)
        {
	    print($pfx, "State $i: '$meta->{states}->[$i]'\n");
	}
    }
}
    
# Dump result of archiver.values()
sub show_values($)
{
    my ($results) = @ARG;
    my (%meta, $result, $time, $stat, $value);
    foreach $result ( @{$results} )
    {
	print("Result for channel '$result->{name}':\n");
	show_meta("", $result->{meta});
	print("Type: $result->{type}, element count $result->{count}.\n");	
	foreach $value ( @{$result->{values}} )
	{
	    $time = time2string($value->{secs}, $value->{nano});
	    $stat = stat2string($value->{stat}, $value->{sevr});
	    print("$time @{$value->{value}} $stat\n");
	}
    }
}

# Dump result of archiver.values(), works only
# for how = spreadsheet
sub show_values_as_sheet($)
{
    my ($results) = @ARG;
    my (%meta, $result, $stat, $channels, $vals, $v, $c, $i);
    $channels = $#{$results} + 1;
    return if ($channels <= 0);
    $vals = $#{$results->[0]->{values}} + 1;
    # Dumping the meta information as a spreadsheet comment
    foreach $result ( @{$results} )
    {
	print("# Channel '$result->{name}':\n");
	show_meta("# ", $result->{meta});
	print("# Type: $result->{type}, element count $result->{count}.\n");
    }
    # Header: "Time" & channel names
    print("# Time\t");
    for ($c=0; $c<$channels; ++$c)
    {
	print("$results->[$c]->{name}\t");
	print("[$results->[$c]->{meta}->{units}]")
	    if ($results->[$c]->{meta}->{type} == 1);
	print("\t");
    }
    print("\n");
    # Spreadsheet cells
    for ($v=0; $v<$vals; ++$v)
    {
	for ($c=0; $c<$channels; ++$c)
	{
	    if ($c == 0)
	    {
		print(time2string($results->[$c]->{values}->[$v]->{secs},
				  $results->[$c]->{values}->[$v]->{nano}),
		      "\t");
	    }
	    $stat = stat2string($results->[$c]->{values}->[$v]->{stat},
				$results->[$c]->{values}->[$v]->{sevr});
	    print("@{$results->[$c]->{values}->[$v]->{value}}\t$stat");
	    if ($c == $channels-1)
	    {
		print("\n");
	    }
	    else
	    {
		print("\t");
	    }
	}
    }
}

# ----------------------------------------------------------------------
# The main routine
# ----------------------------------------------------------------------

if (!getopts('vu:iak:lm:h:s:e:c:') or
    ($#ARGV < 0 and not ($opt_i or $opt_a or $opt_l or $opt_m)))
{
    usage();
}

# All but -a and -i require -k:
if ((length($opt_k) <= 0) and not ($opt_a or $opt_i))
{
    print("You need to specify an archive key (-k option)\n");
    exit(-1);
}

$opt_u = 'http://localhost/archive/cgi/ArchiveDataServer.cgi'
    unless (length($opt_u) > 0);

print("Connecting to Archive Data Server URL '$opt_u'\n") if ($opt_v);
$server = Frontier::Client->new(url => $opt_u);

if ($opt_i)
{
    $result = $server->call('archiver.info');
    printf("Archive Data Server V %d\n", $result->{'ver'});
    printf("Description:\n");
    printf("%s", $result->{'desc'});
    my ($array) = $result->{'how'};
    my ($i);
    printf("Supports requests with how = ...\n");
    for ($i=0; $i<=$#{$array}; ++$i)
    {
	printf("%3d: '%s'\n", $i, $array->[$i]);
    }
    $array = $result->{'stat'};
    printf("Alarm Status Strings = ...\n");
    for ($i=0; $i<=$#{$array}; ++$i)
    {
	printf("%3d: '%s'\n", $i, $array->[$i]);
    }
    printf("Alarm Severity Strings = ...\n");
    printf("Num  Severity             Has Value?  Status Text?\n");
    foreach my $stat ( @{ $result->{'sevr'} } )
    {
	printf("%4d %-20s %-5s       %-5s\n",
	       $stat->{num},
	       $stat->{sevr},
	       $stat->{has_value}->value() ? "true" : "false",
	       $stat->{txt_stat}->value()  ? "true" : "false");
    }
}
elsif ($opt_a)
{
    print("Archives:\n");
    $results = $server->call('archiver.archives', "");
    foreach $result ( @{$results} )
    {
	$key = $result->{key};
	print("Key $key: '$result->{name}' in '$result->{path}'\n");
    }
}
elsif ($opt_l or $opt_m)
{
    print("Channels:\n");
    $results = $server->call('archiver.names', $opt_k, $opt_m);
    foreach $result ( @{$results} )
    {
	my ($name) = $result->{name};
	my ($start) = time2string($result->{start_sec}, $result->{start_nano});
	my ($end)   = time2string($result->{end_sec},   $result->{end_nano});
	print("Channel $name, $start - $end\n");
    }
}
else
{
    if (length($opt_s) < 10  or
	length($opt_e) < 10)
    {
	print("You need to specify a start and end time, options -s and -e\n");
	exit(-1);
    }
    my ($start, $startnano) = string2time($opt_s);
    my ($end, $endnano)   = string2time($opt_e);
    $opt_c = 10 unless (defined($opt_c));
    $opt_h = 1 unless (defined($opt_h));
    # note: have to pass ref. to the 'names' array,
    # otherwise perl will turn it into a sequence of names:
    $results = $server->call('archiver.values', $opt_k, \@ARGV,
			     $start, $startnano, $end, $endnano,
			     $opt_c, $opt_h);
    if ($opt_h == 1)
    {
	show_values_as_sheet($results);
    }
    else
    {
	show_values($results);
    }
}


