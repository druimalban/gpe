SUBDIRS = po

INCLUDES = $(GPE_BLUETOOTH_CFLAGS) -I$(top_srcdir)/gpe -I$(top_srcdir) \
           -DPREFIX=\"@prefix@\" -D_GNU_SOURCE -DGPE_BLUETOOTH \
	   -DDBUS_API_SUBJECT_TO_CHANGE \
           -DPACKAGE_LOCALE_DIR=\"@gpebluetooth_localedir@\"

bin_PROGRAMS = gpe-bluetooth bnep-helper rfcomm-helper

gpe_bluetooth_LDADD = @GPE_BLUETOOTH_LIBS@ -lopenobex -lbluetooth -lsqlite
gpe_bluetooth_SOURCES = main.c dun.c sdp.c pan.c lap.c rfcomm.c pin-ui.c dbus.c pin-dbus.c headset.c obexserver.c obex-vcard.c obex-vcal.c obex-unknown.c obexclient.c obex-glib.c obexclient-dbus.c
bnep_helper_SOURCES = bnep-helper.c
bnep_helper_LDADD = -lbluetooth
rfcomm_helper_SOURCES = rfcomm-helper.c

desktopdir = $(datadir)/applications
desktop_in_files = gpe-bluetooth.desktop.in

servicedir =
service_in_files =

#pixmaps_extra_DATA = pixmaps/gpe-bluetooth.png

desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@
service_DATA = $(service_in_files:.service.in=.service)

pixmapsdir = $(datadir)/gpe-bluetooth

pixmaps_DATA = pixmaps/bt-off.png \
               pixmaps/bt-on.png \
               pixmaps/cellphone.png \
               pixmaps/Computer.png \
               pixmaps/Printer.png \
	       pixmaps/network.png

pixmaps_extradir = $(datadir)/pixmaps

EXTRA_DIST = $(desktop_DATA) \
             $(pixmaps_DATA) \
	     $(service_DATA) \
             $(pixmaps_extra_DATA) \
             intltool-extract.in \
             intltool-merge.in \
             intltool-update.in

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

