#!/usr/bin/perl
print "Content-type: text/html\n";
print "\n"; 
print "<HTML>\n";
print "<HEAD>\n";
print "<TITLE>Environment Dump</TITLE>\n";
print "</HEAD>\n";
print "<BODY BGCOLOR=\"#FFFFFF\" VLINK=\"#0000FF\" BACKGROUND=\"../blueback.jpg\">\n";
print "<FONT FACE=\"Comic Sans MS, Arial, Helvetica\">\n";
print "<H1>Environment Dump</H1>\n";
print "<TABLE CELLPADDING=2 CELLSPACING=1 WIDTH=0 BORDER=1>\n";
foreach $name ( keys %ENV )
{
	print "<TR><TD>$name</TD><TD>$ENV{$name}</TD></TR>\n";
}
print "</TABLE>\n";

print "</FONT>\n";
print "</BODY>\n";
print "</HTML>\n";

