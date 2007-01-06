#!/usr/bin/perl

# gpe-fetchmap
#
# Based on gpsfetchmap by: Kevin Stephens
# Based on shell script by: Kevin Stephens and Fritz Ganter and Manfred Carus
#

use strict;
use Getopt::Long;
use Pod::Usage;

# Setup possible scales
my @SCALES = (10000,20000,40000,160000,640000);

# Set defaults and get options from command line
Getopt::Long::Configure('no_ignore_case');
my ($lat,$lon,$slat,$endlat,$slon,$endlon,$force,$area,$mapdir,$help);
my $FILEPREFIX    = 'map_';
my $mapserver     = 'mapblast';

GetOptions ('lat=f' => \$lat, 'lon=f' => \$lon, 'area=s' => \$area, 'help|x' => \$help)
   or pod2usage(1);

pod2usage(1) if $help;

# Verify that we have the options that we need 
pod2usage(1) if (&error_check);

# Setup up some constants
my $DIFF          = 0.000001;
my $KM2MILES      = 0.62137119;
my $RADIUS_KM     = 6371.01;
my $LAT_DIST_KM   = 110.87;

print "Centerpoint: $lat,$lon\n";

# Get mapdir from config file, unless they override with command line
$mapdir = "$ENV{'HOME'}/.maps";

# Now get the start and end coordinates
unless ($slat && $slon && $endlat && $endlon) {
   ($slat,$slon,$endlat,$endlon) = get_coords(\$lat,\$lon,\$area); 
}
print "Upper left: $slat $slon, Lower Right: $endlat, $endlon\n";

unless ($force) {
   my $count = file_count(\($slat,$slon,$endlat,$endlon));
   print "You are about to download $count file(s).\nAre you sure you want to continue? [y|n] ";
   my $answer = <STDIN>;
   exit if ($answer !~ /^[yY]/);    
}

print "\nDownloading files:\n";

# Change into the gpsdrive maps directory 
`mkdir -p $mapdir`;
chdir($mapdir);

my $incscale = 1;
my $incx = 0;
my $incy = 0;
my $filename_dat = "map.dat";
`rm -f $filename_dat`;

# Ok start getting the maps
foreach my $scale (@SCALES) {
   # Setup k
   my $k = $DIFF * $scale;
   my $lati = $slat;   
   $incy = -1;
   while ($lati < $endlat) {
      my $long = $slon;
      $incx = -1;
      $incy += 1;
      while ($long < $endlon) {
         $incx += 1;
	 my $filename = "map-$incscale-$incx-$incy.gif";
         if (! -s $filename) {
            LOOP: {
	       my $lareal = sprintf("%.4f", $lati);
	       my $loreal = sprintf("%.4f", $long);
	       print "wget -nd -q -O $filename \"http://www.mapblast.com/myblastd/MakeMap.d?\&CT=$lareal:$loreal:$scale\&IC=\&W=240\&H=320\&LB=\"\n";
               `wget -nd -q -O $filename "http://www.mapblast.com/myblastd/MakeMap.d?\&CT=$lareal:$loreal:$scale\&IC=\&W=240\&H=320\&LB="`;                       
            }
         }
         $long += $k;
      }
      $lati += $k; 
   }
   `echo "$incscale $incx $incy" >> $filename_dat`; 
   $incscale+=1;
}
print "\n";

################################################################################
#
# Subroutines
#
################################################################################

sub error_check {
   my $status;
   
   # Check for a centerpoint
   unless (($lat && $lon)) {
      print "ERROR: You must supply latitude and longitude coordinates\n\n";
      $status++;
   }
   
   # Check for area
   unless ($area) {
      print "ERROR: You must define an area to cover\n\n";
      $status++;
   }
   
   return $status;
}

sub file_count {
   my ($slat,$slon,$endlat,$endlon) = @_;
   my $count;
   foreach my $scale (@SCALES) {
      my $k = $DIFF * $scale;
      my $lati = $$slat;   
      while ($lati < $$endlat) {
         my $long = $$slon;
         while ($long < $$endlon) {
            $long += $k;
            $count++;
         }
         $lati += $k;
      }
   }
   return($count);
} #end file_count

sub get_coords {
   my ($lat_ref,$lon_ref,$area_ref) = @_;

   # Figure out if we are doing square area or a rectangle
   my ($lat_dist,$lon_dist);
   $lat_dist = $$area_ref;
   $lon_dist = $$area_ref;
   
   print "Latitude distance: $lat_dist, Longitude distance: $lon_dist\n"; 

   my $lon_dist_km = calc_lon_dist($lat_ref);
   my $lat_offset  = calc_offset(\($lat_dist,$LAT_DIST_KM));
   my $lon_offset  = calc_offset(\($lon_dist,$lon_dist_km));   

   print "LAT_OFFSET = $$lat_offset LON_OFFSET = $$lon_offset \n";
   
   # Ok subtract the offset for the start point
   my $slat = $$lat_ref - $$lat_offset;
   my $slon = $$lon_ref - $$lon_offset;
   
   # Ok add the offset for the start point
   my $elat = $$lat_ref + $$lat_offset;   
   my $elon = $$lon_ref + $$lon_offset;   
    
   return ($slat,$slon,$elat,$elon);
} #End get_coords

sub calc_offset {
   my($area_ref,$dist_per_degree) = @_;
   
   # Adjust the dist_per_degree for the unit chosen by the user
   $$dist_per_degree *= $KM2MILES;   
   
   # The offset for the coordinate is the distance to travel divided by 
   # the dist per degree   
   my $offset = sprintf("%.7f", ($$area_ref / 2) / $$dist_per_degree);
   
   return(\$offset);
} #End calc_offset

sub calc_lon_dist {
   my ($lat) = @_;
   my $PI  = 3.141592654;
   my $dr = $PI / 180;
   
   # calculate the circumference of the small circle at latitude 
   my $cos = cos($$lat * $dr); # convert degrees to radians
   my $circ_km = sprintf("%.2f",($PI * 2 * $RADIUS_KM * $cos));
   
   # divide that by 360 and you have kilometers per degree
   my $km_deg = sprintf("%.2f",($circ_km / 360));
   
   return ($km_deg);
} #End calc_longitude_dist

__END__

=head1 NAME

B<gpe-fetchmap> Version 0.1

=head1 DESCRIPTION

B<gpe-fetchmap> is a program to download maps from mapblast for use with gpe-mapviewer. 

=head1 SYNOPSIS

B<Usage:>

gpe-fetchmap -la <latitude MM.DDDD> -lo <latitude MM.DDDD> -a <#>

B<All options:>

gpe-fetchmap [-la <latitude DD.MMMM>] [-lo <longitude DD.MMMM>] 
            [-a <#>] [-h] [-M]

=head1 OPTIONS

=over 8

=item B<-la,  --lat <latitude DD.MMMM>>

Takes a latitude in format DD.MMMM and uses that as the latitude for the centerpoint of the area
to be covered. Will be overriden by the latitude of waypoint if '-w' is used. This and '-lo', '-w' or '-sla', '-ela', '-slo', '-elo' is required.

=item B<-lo, --lon <longitude DD.MMMM>>

Takes a longitude in format DD.MMMM and uses that as the longitude for the centerpoint of the area
to be covered. Will be overriden by the longitude of waypoint if '-w' is used. This and '-la', '-w' or '-sla', '-ela', '-slo', '-elo' is required.

=item B<-a, --area <#>>

Area to cover. # of 'units' size square around the centerpoint. You can use a single number
for square area. Or you can use '#x#' to do a rectangle, where the first number is distance
latitude and the second number is distance of longitude. 'units' is read from the configuration 
file (-C) or as defined by (-u).

=item B<--help -h -x>

Prints the usage page and exits.

=back

=cut

