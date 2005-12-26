SUBDIRS = po

INCLUDES = $(BLUEZ_PIN_CFLAGS) -I$(top_srcdir)/gpe -I$(top_srcdir) \
           -DPREFIX=\"@prefix@\" -D_GNU_SOURCE -DDBUS_API_SUBJECT_TO_CHANGE \
           -DPACKAGE_LOCALE_DIR=\"@bluez_pin_localedir@\"

bin_PROGRAMS = bluez-pin

bluez_pin_LDADD = @BLUEZ_PIN_LIBS@
bluez_pin_SOURCES = main.c pin-ui.c pin-dbus.c pin-ui.h

dbusconfdir = $(sysconfdir)/dbus-1/system.d
dbusconf_DATA = bluez.conf

pixmapsdir = $(datadir)/pixmaps
pixmaps_DATA = bt-logo.png

pingladedir = $(datadir)/bluez-pin
pinglade_DATA = bluez-pin.glade bluez-pin-gpe.glade

EXTRA_DIST = $(desktop_DATA) \
             $(pixmaps_DATA) \
	     $(dbusconf_DATA) \
             intltool-extract.in \
             intltool-merge.in \
             intltool-update.in \
	     bluez-pin.glade \
	     bluez-pin-gpe.glade

DISTCLEANFILES = intltool-extract \
                 intltool-merge \
                 intltool-update

clean-local:
	rm -f gpe-bluetooth.desktop
	rm -f po/.intltool-merge-cache 
	rm -rf autom4te.cache
	rm -f *.d *~

include gpe-dist.am
include gpe-tp.am
