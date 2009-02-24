#!/usr/bin/perl
# make_indexconfig.pl
#
# kasemir@lanl.gov

use English;
use strict;
use vars qw($opt_d);
use Getopt::Std;

sub usage()
{
    print("USAGE: make_indexconfig [-d DTD] index { index }\n");
    print("\n");
    print("This tool generates a configuration for the ArchiveIndexTool\n");
    print("based on a DTD and a list of index files provided via\n");
    print("the command line.\n");
    print("The typical use is to create an index config for\n");
    print("a couple of existing e.g. daily sub-archives like this:\n");
    print("   make_indexconfig -d /a/b/indexconfig.dtd 2004/*/index >indexconfig.xml\n");
    exit(-1);
}

if (!getopts('d:')  ||  $#ARGV < 0)
{
    usage();
}

$opt_d = "http://localhost/indexconfig.dtd" unless (length($opt_d) > 0);
print "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\n";
print "<!DOCTYPE indexconfig SYSTEM \"$opt_d\">\n";
print "<indexconfig>\n";
foreach my $index ( @ARGV )
{
    print "\t<archive>\n";
    print "\t\t<index>$index</index>\n";
    print "\t</archive>\n";
}
print "</indexconfig>\n";
