PACKAGE = vtype-utils
VERSION = 0.5
CVSBUILD = yes
DEBUG = no
INSTALL = install
PREFIX = /usr/local
DESTDIR = /
CONTROL = control1

LINGUAS = 

ifeq ($(CVSBUILD),yes)
BUILD = ../build
else
BUILD = build
endif

ifeq ($(DEBUG),yes)
PACKAGE_CFLAGS += -g -DDEBUG -DIRCP_DEBUG=9
else
PACKAGE_CFLAGS += -Os -fomit-frame-pointer 
endif

PACKAGE_CFLAGS += -DVERSION=\"$(VERSION)\" -DPREFIX=\"$(PREFIX)\" -D_GNU_SOURCE -DPACKAGE=\"$(PACKAGE)\" 
PACKAGE_CFLAGS += -DPACKAGE_LOCALE_DIR=\"$(PREFIX)/share/locale\"

PACKAGE_CPPFLAGS += -I.

MIMECFLAGS =`pkg-config --cflags mimedir-1.0`
MIMELDFLAGS = $(GTKLDFLAGS) -lsqlite `pkg-config --libs mimedir-1.0`

ifeq ($(CVSBUILD),yes)
MIMECFLAGS += $(shell PKG_CONFIG_PATH=../libgpevtype pkg-config --cflags libgpevtype-uninstalled)
MIMELDFLAGS += $(shell PKG_CONFIG_PATH=../libgpevtype pkg-config --libs libgpevtype-uninstalled)
else
MIMECFLAGS += `pkg-config --cflags libgpevtype`
MIMELDFLAGS += `pkg-config --libs libgpevtype`
endif

STRIP=strip
CC=gcc

#PACKAGE_CFLAGS +=  -Wall $(GPECFLAGS) $(GTKCFLAGS) `pkg-config --cflags gthread-2.0` `pkg-config --cflags dbus-glib-1`
#PACKAGE_LDFLAGS += $(GPELIBS) $(GTKLIBS) `pkg-config --libs gthread-2.0` `pkg-config --libs dbus-glib-1`


all: vcard-export vcard-import vcal-export

PACKAGE_CFLAGS += $(MIMECFLAGS)

vcal-export: vcal-export.o
	$(CC) -o $@ $^ $(LDFLAGS) $(MIMELDFLAGS)

vcard-export: vcard-export.o
	$(CC) -o $@ $^ $(LDFLAGS) $(MIMELDFLAGS)

vcard-import: vcard-import.o
	$(CC) -o $@ $^ $(LDFLAGS) $(MIMELDFLAGS)

install-program: all 
	mkdir -p $(DESTDIR)/CONTROL
	sed -e s/VERSION/$(VERSION)/ <familiar/$(CONTROL) > $(DESTDIR)/CONTROL/control
	mkdir -p $(DESTDIR)/$(PREFIX)/bin
	$(INSTALL) -m 755 vcal-export $(DESTDIR)$(PREFIX)/bin/
	$(INSTALL) -m 755 vcard-export $(DESTDIR)$(PREFIX)/bin/
	$(INSTALL) -m 755 vcard-import $(DESTDIR)$(PREFIX)/bin/

clean:
	rm -f vcard-export vcard-import vcal-export $(OBJS) *~ */*~ *.ipk *.o 

include $(BUILD)/Makefile.dpkg_ipkg
include $(BUILD)/Makefile.translation
