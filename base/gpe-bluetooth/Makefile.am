SUBDIRS = po

INCLUDES = $(GPE_BLUETOOTH_CFLAGS) -I$(top_srcdir)/gpe -I$(top_srcdir) \
           -DPREFIX=\"@prefix@\" -D_GNU_SOURCE -DGPE_BLUETOOTH \
           -DDBUS_API_SUBJECT_TO_CHANGE \
           -DPACKAGE_LOCALE_DIR=\"@gpebluetooth_localedir@\"

bin_PROGRAMS = gpe-bluetooth bnep-helper rfcomm-helper

gpe_bluetooth_LDADD = @GPE_BLUETOOTH_LIBS@ -lopenobex -lsqlite
gpe_bluetooth_SOURCES = main.c dun.c sdp.c pan.c lap.c rfcomm.c pin-ui.c dbus.c pin-dbus.c headset.c obexserver.c obex-vcard.c obex-vcal.c obex-unknown.c obexclient.c obex-glib.c obexclient-dbus.c main.h sdp.h dun.h dbus.h pan.h lap.h headset.h obexclient.h obexserver.h bnep.h rfcomm.h obex-glib.h pin-ui.h
bnep_helper_SOURCES = bnep-helper.c
bnep_helper_LDADD = -lbluetooth
rfcomm_helper_SOURCES = rfcomm-helper.c

desktopdir = $(datadir)/applications
desktop_in_files = gpe-bluetooth.desktop.in

servicedir =
service_in_files =

dbusconfdir = $(sysconfdir)/dbus-1/system.d
dbusconf_DATA = bluez.conf

peersdir = $(sysconfdir)/ppp/peers
peers_DATA = peers/lap

#pixmaps_extra_DATA = pixmaps/gpe-bluetooth.png

desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@
service_DATA = $(service_in_files:.service.in=.service)

pixmapsdir = $(datadir)/gpe/pixmaps/default/bluetooth

pixmaps_DATA = pixmaps/bt-off.png \
               pixmaps/bt-on.png \
               pixmaps/cellphone.png \
               pixmaps/Computer.png \
               pixmaps/Printer.png \
               pixmaps/network.png

pixmaps_extradir = $(datadir)/pixmaps

btscriptdir = $(sysconfdir)/bluetooth
btscript_SCRIPTS = hciattach.sh

btresumedir = $(sysconfdir)/apm/resume.d
btresume_SCRIPTS = gpe-bluetooth-resume.sh

EXTRA_DIST = $(desktop_DATA) \
             $(pixmaps_DATA) \
             $(service_DATA) \
	         $(dbusconf_DATA) \
	         $(peers_DATA) \
	         $(btscript_SCRIPTS) \
	         $(btresume_SCRIPTS) \
	         gpe-bluetooth.desktop.in \
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
