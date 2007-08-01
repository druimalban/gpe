bin_PROGRAMS = minimix

INCLUDES = @MINIMIX_CFLAGS@  -DPREFIX=\"@prefix@\" \
           -DPACKAGE_LOCALE_DIR=\"@minimix_localedir@\"
           
LDADD = @MINIMIX_LIBS@

desktopdir = $(datadir)/applications
desktop_DATA = minimix.desktop

pixmapsdir = $(datadir)/pixmaps
pixmaps_DATA = stock_volume-0.png \
	stock_volume-max.png \
	stock_volume-med.png \
	stock_volume-min.png \
	stock_volume-mute.png \
	stock_volume.png

EXTRA_DIST = ${desktop_DATA} \
	${pixmaps_DATA}

include gpe-dist.am
