#! /usr/bin/perl
#
# Convert SLAC ioc name .html file to a file with only the ioc names in it. 
# Steps we take to do that are:
#
# AUTH: Ronm
# MODS:
#   0305/08.  RonM.
#             Add hardcoded nodes, for example lcls-daemon1.
#
#
# DESCRIPTION:
# Open Argument 1 which is the input file's name and parse the file names in it.
# Write the ioc names from that file into $IOC_NAMES_LIST_FILE.
# The output file will be overwritten if it already exists.
#
# EXAMPLE of how to run this perl script:
#
# ./parseIocNames /afs/slac/www/grp/cd/soft/database/reports/ioc_report.html
# 
# The output filename will be that thing that EPICS iocLogServer will use for
# the list of throttles (it contains the list of hosts to be throttled).
#
# ------------------------------- code ---------------------------------------
#
# Must have one arg.
if ($#ARGV != 0) {
 print "usage:./parseIocNames slac_input_html_file \n";
 exit;
}
if (!$ENV{IOC_NAMES_LIST_FILE}) {
 print "You must set the output file name environment variable IOC_NAMES_LIST_FILE first\n";
 exit
} 
print "\nExtracting ioc names from $ARGV[0]...\n";
#
open(IN_FILE, $ARGV[0]) || die "Can't open IOC NAMES INPUT HTML FILE: $!\n";
#
$out_file_name=$ENV{IOC_NAMES_LIST_FILE};
print "\nWriting to output file of ioc names: $out_file_name\n\n";
open(OUT_FILE, ">$out_file_name") || die "Can't open IOC NAMES OUTPUT FILE: $!\n";
#
# Loop thru input file a line at a time.  
# If there is an IOC name on the line, output that name.
# We're done when "TOTAL" is encountered.
#
# Here is an example line with an IOC in it from the input file:
#       <td width="45" height="10" bgcolor="#FFFFFF">ioc-in20-tm01</td>
#
while($line = <IN_FILE>){
    last if ($line =~ /TOTAL/);
    if ($line =~ m/FFFFF">(.*)<\/td>/) {
#	print "$1\n";
        print OUT_FILE "$1\n";
    }
}
#
# ADD IOC NAMES THAT ARE NOT IN JUDY"S FILE. Examples are development ioc's that are
# booted into the production system and ioc's that are build in some other epics tree
# than Judy's IRMIS program is scanning.
#
	print OUT_FILE "lclstst-03\n";
	print OUT_FILE "lclstst-01\n";
	print OUT_FILE "ioc-in20-rf01\n";
	print OUT_FILE "slcsun1\n";
        print OUT_FILE "lcls-builder\n";
        print OUT_FILE "lcls-daemon1\n";
        print OUT_FILE "lcls-prod02\n";
        print OUT_FILE "lcls-srv01\n";
        print OUT_FILE "lcls-srv02\n";
#
close(IN_FILE);
close(OUT_FILE);
