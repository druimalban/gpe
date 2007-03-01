
SUBDIRS = po

INCLUDES = $(DEPS_CFLAGS) -I$(top_srcdir)/gpe -I$(top_srcdir) -I$(top_srcdir)/modules \
           -DPREFIX=\"@prefix@\" -D_GNU_SOURCE -Wall \
           -DPACKAGE_LOCALE_DIR=\"@gpeconf_localedir@\" \
           -DVERSION=\"$(VERSION)\" \
           -DDBUS_API_SUBJECT_TO_CHANGE

#LDFLAGS = -pg
#INCLUDES = $(INCLUDES) -pg

pcmcia_headers = pcmcia/version.h pcmcia/cs_types.h \
                 pcmcia/cs.h pcmcia/cistpl.h pcmcia/ds.h \
                 pcmcia/bulkmem.h pcmcia/driver_ops.h

gpe_conf_SOURCES = main.c applets.c parser.c suid.c misc.c \
                modules/timeanddate.c \
                modules/screen/screen.c modules/screen/brightness.c  \
                modules/screen/rotation.c modules/screen/calibrate.c \
                modules/screen/callbacks.c modules/screen/xset.c \
                modules/keys/keyboard.c modules/keys/kbd.c \
                modules/keys/keys.c modules/keys/keyctl.c \
                modules/network.c \
                modules/theme.c \
                modules/ownerinfo.c \
                modules/login-setup.c \
                modules/sleep/sleep.c modules/sleep/sinterface.c \
                modules/sleep/scallbacks.c modules/sleep/conf.c modules/sleep/confGUI.c \
                modules/users/uinterface.c modules/users/ucallbacks.c \
                modules/users/passwd.c \
                modules/cfgfile.c \
                modules/gpe-admin.c \
                modules/serial.c \
                modules/usb.c \
                modules/cardinfo.c \
                tools/interface.c \
                modules/tasks.c \
                modules/sound/sound.c modules/sound/soundctrl.c \
                modules/sound/alarmctrl.c \
                device.c \
                applets.h parser.h suid.h misc.h \
                modules/timeanddate.h modules/timezones.h \
                modules/screen/screen.h modules/screen/brightness.h  \
                modules/screen/rotation.h modules/screen/calibrate.h \
                modules/screen/callbacks.h modules/screen/xset.h \
                modules/keys/keyboard.h modules/keys/kbd.h \
                modules/keys/keys.h modules/keys/keyctl.h \
                modules/network.h \
                modules/theme.h \
                modules/ownerinfo.h \
                modules/login-setup.h \
                modules/sleep/sleep.h modules/sleep/sinterface.h \
                modules/sleep/scallbacks.h modules/sleep/conf.h modules/sleep/confGUI.h \
                modules/users/uinterface.h modules/users/ucallbacks.h \
                modules/users/passwd.h modules/users/users.h \
                modules/cfgfile.h \
                modules/gpe-admin.h \
                modules/serial.h \
                modules/usb.h \
                modules/cardinfo.h \
                tools/interface.h \
                modules/tasks.h \
                modules/sound/sound.h modules/sound/soundctrl.h \
                modules/sound/alarmctrl.h \
                device.h \
                $(pcmcia_headers)

gpe_info_SOURCES = \
               main-info.c \
               modules/storage.c \
               modules/logread.c \
               tools/interface.c \
               modules/battery.c \
               modules/sysinfo.c \
               parser.c \
               device.c \
               modules/storage.h \
               modules/logread.h \
               tools/interface.h \
               modules/battery.h \
               modules/sysinfo.h \
               parser.h \
               device.h

bin_PROGRAMS = gpe-conf gpe-info

gpe_conf_LDADD = $(DEPS_LIBS) -lcrypt
gpe_info_LDADD = $(DEPS_LIBS) 

desktopdir = $(datadir)/applications
desktop_in_files = desktop/gpe-conf.desktop.in \
	desktop/gpe-conf-admin.desktop.in \
	desktop/gpe-conf-cardinfo.desktop.in \
	desktop/gpe-conf-screen.desktop.in \
	desktop/gpe-conf-keys.desktop.in \
	desktop/gpe-conf-login-setup.desktop.in \
	desktop/gpe-conf-network.desktop.in \
	desktop/gpe-conf-ownerinfo.desktop.in \
	desktop/gpe-conf-serial.desktop.in \
	desktop/gpe-conf-sleep.desktop.in \
	desktop/gpe-conf-theme.desktop.in \
	desktop/gpe-conf-time.desktop.in \
	desktop/gpe-conf-users.desktop.in \
	desktop/gpe-conf-sysinfo.desktop.in \
	desktop/gpe-conf-usb.desktop.in \
	desktop/gpe-conf-sound.desktop.in

desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

xscriptsdir = $(sysconfdir)/X11/Xsession.d
xscripts_SCRIPTS = modules/sound/soundsettings.sh

pixmapsdir = $(datadir)/pixmaps

pixmaps_DATA = pixmaps/gpe-config-apm.png \
               pixmaps/gpe-config-screen.png \
               pixmaps/gpe-config-kbd.png \
               pixmaps/gpe-config-keyctl.png \
               pixmaps/gpe-config-mixer.png \
               pixmaps/gpe-config-mouse.png \
               pixmaps/gpe-config-network.png \
               pixmaps/gpe-config-ownerinfo.png \
               pixmaps/gpe-config-screensaver.png \
               pixmaps/gpe-config-sleep.png \
               pixmaps/gpe-config-software.png \
               pixmaps/gpe-config-theme.png \
               pixmaps/gpe-config-time.png \
               pixmaps/gpe-config.png \
               pixmaps/gpe-config-users.png \
               pixmaps/gpe-config-login-setup.png \
               pixmaps/gpe-config-admin.png \
               pixmaps/gpe-config-storage.png \
               pixmaps/gpe-config-serial.png \
               pixmaps/gpe-config-cardinfo.png \
               pixmaps/pccard-cdrom.png pixmaps/pccard-ide.png pixmaps/pccard-memory.png \
               pixmaps/pccard-parport.png pixmaps/pccard-scsi.png pixmaps/pccard-unknown.png \
               pixmaps/pccard-network.png pixmaps/pccard-usb.png pixmaps/pccard-serial.png \
               pixmaps/gpe-config-battery.png \
               pixmaps/gpe-config-sysinfo.png \
               pixmaps/ibutton1.png pixmaps/ibutton2.png pixmaps/ibutton3.png \
               pixmaps/ibutton4.png pixmaps/ibutton5.png \
               pixmaps/system-info.png \
               pixmaps/distribution.png \
               pixmaps/device-info.png \
               pixmaps/gpe-config-sound.png \
               pixmaps/ipaq-s1.png \
               pixmaps/c7x0-s1.png

sysgpedir = $(sysconfdir)/gpe
sysgpe_DATA = familiar/gpe-config.conf \
              data/key-commands \
              data/key-layout.ipaq \
              data/key-layout.zaurus-clamshell

appfilesdir = $(datadir)/gpe-conf
appfiles_DATA = data/activate.wav \
                data/deviceinfo

EXTRA_DIST = $(sysgpe_DATA) \
             $(pixmaps_DATA) \
             $(xscripts_SCRIPTS) \
             $(appfiles_DATA) \
             $(desktop_in_files) \
             intltool-extract.in \
             intltool-merge.in \
             intltool-update.in


DISTCLEANFILES = intltool-extract \
                 intltool-merge \
                 intltool-update
                 
clean-local:
	rm -rf autom4te.cache
	rm -f *.d *~

install-data-hook:
	chmod u+s $(DESTDIR)$(bindir)/gpe-conf                
	cd $(DESTDIR)/$(xscriptsdir) && mv soundsettings.sh 30soundsettings

deb: dist
	-mkdir $(top_builddir)/debian-build
	cd $(top_builddir)/debian-build && tar xzf $(top_builddir)/../$(PACKAGE)-$(VERSION).tar.gz
	cp -r $(top_builddir)/debian $(top_builddir)/debian-build/$(PACKAGE)-$(VERSION)
	cd $(top_builddir)/debian-build/$(PACKAGE)-$(VERSION) && dpkg-buildpackage -rfakeroot
	-rm -rf $(top_builddir)/debian-build/$(PACKAGE)-$(VERSION)

include gpe-dist.am
include gpe-tp.am
