PREFIX = /usr/local
PACKAGE = hotplug-dbus
DEBUG = no
CVSBUILD = yes
VERSION = 0.9

ifeq ($(CVSBUILD),yes)
BUILD = ../build
else
BUILD = build
endif

all:

install-program:
	install -d $(DESTDIR)/sbin
	install hotplug-dbus $(DESTDIR)/sbin
	install -d $(DESTDIR)/etc/dbus-1/system.d
	install -m 644 hotplug.conf $(DESTDIR)/etc/dbus-1/system.d
	install -d $(DESTDIR)/etc/hotplug.d/default
	ln -sf /sbin/hotplug-dbus $(DESTDIR)/etc/hotplug.d/default/11-dbus.hotplug

clean:

include $(BUILD)/Makefile.translation
include $(BUILD)/Makefile.dpkg_ipkg
