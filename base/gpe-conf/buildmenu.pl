#!/usr/bin/perl -w
#this perlscript makes menu and copy the icon into the right place
# invoque it by ./gpe-conf jhdgkjhd | ./buildmenu.pl >dist/usr/lib/menu/gpe-conf

print "?package(gpe-conf): \\\n";
print "    needs=x11 \\\n";
print "    title=\"GPE Configuration Tools\" \\\n";
print "    command=gpe-conf \\\n";
print "    icon48=/usr/share/pixmaps/gpe-config.png \\\n";
print "    section=Configuration\n\n";
system("cp -f pixmaps/gpe-config.png dist/usr/share/pixmaps/gpe-config.png");

while ($_=<STDIN>)
  {
    chomp;
    if (/([^\t]*)\t\t:(.*)/) {
      $modulename = $1;
      $moduletitle = $2;
      print "?package(gpe-conf): \\\n";
      print "    needs=x11 \\\n";
      print "    title=\"$moduletitle\" \\\n";
      print "    command=\"gpe-conf $modulename\" \\\n";
      print "    icon48=/usr/share/pixmaps/gpe-config-$modulename.png \\\n";
      print "    section=Configuration\n\n";
      if(! -e "pixmaps/gpe-config-$modulename.png"  ){
	
	print STDERR "Warning:  pixmaps/gpe-config-$modulename.png doesnt exists!\n\n";
      }
      else
	{
	  system("cp -f pixmaps/gpe-config-$modulename.png dist/usr/share/pixmaps/gpe-config-$modulename.png");
	}
	
    }
  }
