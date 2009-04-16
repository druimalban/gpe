SUBDIRS = po

INCLUDES = $(GPE_BLUETOOTH_CFLAGS) -I$(top_srcdir)/gpe -I$(top_srcdir) \
           -DPREFIX=\"@prefix@\" -D_GNU_SOURCE -DGPE_BLUETOOTH \
           -DDBUS_API_SUBJECT_TO_CHANGE \
           -DPACKAGE_LOCALE_DIR=\"@gpebluetooth_localedir@\"

bin_PROGRAMS = gpe-bluetooth
gpebluetoothlibdir = $(libdir)/gpe-bluetooth
gpebluetoothlib_PROGRAMS = bnep-helper rfcomm-helper


gpe_bluetooth_LDADD = @GPE_BLUETOOTH_LIBS@ -lopenobex -lsqlite
gpe_bluetooth_SOURCES = main.c dun.c sdp.c pan.c lap.c rfcomm.c pin-ui.c dbus.c pin-dbus.c headset.c obexserver.c obex-vcard.c obex-vcal.c obex-unknown.c obexclient.c obex-glib.c obexclient-dbus.c main.h sdp.h dun.h dbus.h pan.h lap.h headset.h obexclient.h obexserver.h bnep.h rfcomm.h obex-glib.h pin-ui.h progress.c progress.h
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

desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@
service_DATA = $(service_in_files:.service.in=.service)

pixmapsdir = $(datadir)/pixmaps/gpe/default/bluetooth

pixmaps_DATA = pixmaps/bt-off.png \
               pixmaps/bt-on.png \
               pixmaps/cellphone.png \
               pixmaps/Computer.png \
               pixmaps/Printer.png \
               pixmaps/network.png

pixmaps_extradir = $(datadir)/pixmaps

btscriptdir = $(sysconfdir)/bluetooth
btscript_SCRIPTS = hciattach.sh

btresumedir = $(sysconfdir)/apm/scripts.d
btresume_SCRIPTS = gpe-bluetooth-resume.sh

pingladedir = $(datadir)/bluez-pin
pinglade_DATA = bluez-pin-gpe.glade

EXTRA_DIST = $(desktop_DATA) \
             $(pixmaps_DATA) \
             $(service_DATA) \
	     $(dbusconf_DATA) \
	     $(peers_DATA) \
	     $(btscript_SCRIPTS) \
	     $(btresume_SCRIPTS) \
	     $(pinglade_DATA) \
	     gpe-bluetooth.desktop.in \
             intltool-extract.in \
             intltool-merge.in \
             intltool-update.in \
	     mkinstalldirs \
	     po/gpe-bluetooth.pot

DISTCLEANFILES = intltool-extract \
                 intltool-merge \
                 intltool-update

clean-local:
	$(RM) gpe-bluetooth.desktop
	$(RM) po/.intltool-merge-cache 
	$(RM) -r autom4te.cache
	$(RM) *.d *~
	$(RM) po/stamp-it

include gpe-dist.am
include gpe-tp.am

install-data-hook:
	cd $(DESTDIR)$(sysconfdir)/bluetooth && mv hciattach.sh hciattach
	chmod u+s $(DESTDIR)$(gpebluetoothlibdir)/bnep-helper
	chmod u+s $(DESTDIR)$(gpebluetoothlibdir)/rfcomm-helper
	mkdir -p $(DESTDIR)$(sysconfdir)/apm/resume.d
	$(LN_S) $(btresumedir)/$(btresume_SCRIPTS) $(DESTDIR)$(sysconfdir)/apm/resume.d/50-$(btresume_SCRIPTS:.sh=)

