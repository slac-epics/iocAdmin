use English;
use Benchmark;
use Frontier::Client;

# Setup URL
$server_url = 'http://localhost/cgi-bin/xmlrpc/DummyServer.cgi';
$server_url = 'http://localhost/cgi-bin/xmlrpc/ArchiveServer.cgi';
#$server_url = 'http://bogart.ta53.lanl.gov/cgi-bin/xmlrpc/DummyServer.cgi';
$server = Frontier::Client->new(url => $server_url);

$count = $ARGV[0];

@names = ( "fred", "freddy", "Jimmy", "James" );
$end = time();
$start = $end - $count;
$how = 0;

$t0 = new Benchmark();
$results = $server->call('archiver.get_values', \@names,
			 $start, 2, $end, 4, $count, $how);
$t1 = new Benchmark();
$values = 0;
foreach $result ( @{$results} )
{
	@val_array = @{$result->{values}};
	$values = $values + $#val_array + 1;
}
$td = timediff($t1, $t0);
$time = timestr($td);
printf("4 channels, %5d values total in %s\n", $values, $time);

