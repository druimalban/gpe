PREFIX = /usr/local
PACKAGE = hotplug-dbus
DEBUG = no
CVSBUILD = yes
VERSION = 0.3

ifeq ($(CVSBUILD),yes)
BUILD = ../build
else
BUILD = build
endif

all:

install-program:
	install -d $(DESTDIR)/etc/init.d
	install -d $(DESTDIR)/etc/rcS.d
	install -d $(DESTDIR)/sbin
	install -d $(DESTDIR)/etc/dbus-1/system.d
	install hotplug-dbus.init $(DESTDIR)/etc/init.d/hotplug-dbus
	install hotplug-dbus $(DESTDIR)/sbin
	install -m 644 hotplug.conf $(DESTDIR)/etc/dbus-1/system.d
	ln -s ../init.d/hotplug-dbus $(DESTDIR)/etc/rcS.d/S01hotplug-dbus

clean:

include $(BUILD)/Makefile.translation
include $(BUILD)/Makefile.dpkg_ipkg
