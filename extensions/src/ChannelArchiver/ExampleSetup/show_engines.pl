#!/usr/bin/perl
#
# Filter the output of 'ps' for ArchiveEngine info
#
# kasemirk@ornl.gov

$limit = 0.0;
$cpu_total = 0.0;
$mem_total = 0.0;
%used_ports={};

open(PS, "ps -aux|") or die "Cannot run ps";
while (<PS>)
{
	chomp;
# Output of ps looks like this:
# USER       PID %CPU %MEM   VSZ  RSS TTY      STAT START   TIME COMMAND
# kasemir  24327  1.6  2.1 1318876 10968 ?     S    08:01   0:36 ArchiveEngine -d iocHealth -l 20040824080108.log -p 4601 iocHealth-group.xml 2004/08_24/index
	next unless m/(\S+)\s+(\S+)\s+(\S+)\s+(\S+)\s+(\S+)\s+(\S+)\s+(\S+)\s+(\S+)\s+(\S+)\s+(\S+)\s+ArchiveEngine (.+)/;
	$user = $1;
	$pid = $2;
	$cpu = $3;
	$mem = $4;
	$vsz = $5;
	$rss = $6;
	$tty = $7;
	$stat = $8;
	$start = $9;
	$time = $10;
	$command = $11;
	if ($command =~ m/-d (.+) -l .* -p ([0-9]+) /)
        {
		$port = $2;
		if ($used_ports{$port})
		{
			print("FATAL: Port $port is used by more than one engine!\n");
			print("       PID $used_ports{$port} as well as $pid\n");
			print("\n");
		}
		$used_ports{$port} = $pid;
	}
	$limit = $cpu if ($cpu > $limit);
	$limit = $mem if ($mem > $limit);
	# Add info to array of hashes
	$info = {};
	$info->{cpu} = $cpu;
	$info->{mem} = $mem;
	$info->{user} = $user;
	$info->{pid} = $pid;
	$info->{start} = $start;
	$info->{port} = $port;
	$info->{cmd} = $command;
	push @engines, $info;
	$cpu_total += $cpu;
	$mem_total += $mem;
}


printf "*   CPU   MEM USER       PID     PORT    COMMAND (ArchiveEngine ...)\n";
foreach $info ( @engines )
{
    if ($info->{cpu} >= $limit  ||  $info->{mem} >= $limit)
    {
	print("!!");
    }
    else
    {
	print("  ");
    }
    printf("%5.1f %5.1f %-10s %-7s %-7s %-40s\n",
	   $info->{cpu}, $info->{mem}, $info->{user}, $info->{pid}, $info->{port}, $info->{cmd});
}
printf "\n";
$num = $#engines + 1;
printf "* A total of $num engines is running,\n";
printf "  using $cpu_total %% of the CPU and $mem_total %% of the memory.\n";
printf "  First column marks engines with highest CPU/MEM usage\n";

