PACKAGE = gpe-beam
VERSION = 0.2.9
CVSBUILD = yes
DEBUG = no
INSTALL = install
PREFIX = /usr/local
DESTDIR = /
CONTROL = control1

LINGUAS = cs de sv ro nl pt ru fr sr es pt_BR zh_CN vi rw sl fi

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
PACKAGE_CFLAGS += -DPACKAGE_LOCALE_DIR=\"$(PREFIX)/share/locale\" -DDBUS_API_SUBJECT_TO_CHANGE

PACKAGE_CPPFLAGS += -I. -Wall

MIMECFLAGS =`pkg-config --cflags mimedir-1.0`
MIMELDFLAGS = $(GTKLDFLAGS) -lsqlite `pkg-config --libs mimedir-1.0`

ifeq ($(CVSBUILD),yes)
MIMECFLAGS += $(shell PKG_CONFIG_PATH=../libgpevtype pkg-config --cflags libgpevtype-uninstalled)
MIMELDFLAGS += $(shell PKG_CONFIG_PATH=../libgpevtype pkg-config --libs libgpevtype-uninstalled)
else
MIMECFLAGS += `pkg-config --cflags libgpevtype`
MIMELDFLAGS += `pkg-config --libs libgpevtype`
endif

libircp_a_OBJECTS =  ircp/ircp_client.o ircp/ircp_server.o ircp/ircp_io.o ircp/dirtraverse.o
IRFLAGS = -lopenobex libircp.a

STRIP=strip
CC=gcc
AR = ar
RANLIB = ranlib

PACKAGE_CFLAGS +=  -Wall $(GPECFLAGS) $(GTKCFLAGS) `pkg-config --cflags gthread-2.0` `pkg-config --cflags dbus-glib-1`
PACKAGE_LDFLAGS += $(GPELIBS) $(GTKLIBS) `pkg-config --libs gthread-2.0` `pkg-config --libs dbus-glib-1`

MEMBERS  = main filesel dbus obexclient-dbus

OBJS = $(patsubst %,%.o,$(MEMBERS))
DEPS = $(patsubst %,%.d,$(MEMBERS))
SOURCES = $(patsubst %,%.c,$(MEMBERS))


PIXMAPS = irda-16.png irda.png irda-on-16.png irda-conn-16.png irda-on-48.png irda-conn-48.png

all: $(PACKAGE) irsw

$(PACKAGE): libircp.a $(OBJS)
	$(CC) -g -o $@ $(OBJS) $(LDFLAGS) $(IRFLAGS) $(PACKAGE_LDFLAGS)

irsw: irsw.o irsw.o
	$(CC) -o $@ $^ $(LDFLAGS)

PACKAGE_CFLAGS += $(MIMECFLAGS)

libircp.a: $(libircp_a_OBJECTS) $(libircp_a_DEPENDENCIES)
	-rm -f libircp.a
	$(AR) cru libircp.a $(libircp_a_OBJECTS) $(libircp_a_LIBADD)
	$(RANLIB) libircp.a

install-program: all all-desktop
	mkdir -p $(DESTDIR)/CONTROL
	sed -e s/VERSION/$(VERSION)/ <familiar/$(CONTROL) > $(DESTDIR)/CONTROL/control
	mkdir -p $(DESTDIR)/$(PREFIX)/bin
	$(INSTALL) $(PACKAGE) $(DESTDIR)$(PREFIX)/bin/
	$(INSTALL) -m 4755 irsw $(DESTDIR)$(PREFIX)/bin/
	mkdir -p $(DESTDIR)$(PREFIX)/share/pixmaps
	for i in $(PIXMAPS); do install -m 644 -D pixmaps/$$i $(DESTDIR)$(PREFIX)/share/pixmaps/$$i; done
	mkdir -p $(DESTDIR)$(PREFIX)/share/applications
	$(INSTALL) -m 644 *.desktop $(DESTDIR)$(PREFIX)/share/applications

clean:
	rm -f $(PACKAGE) vcard-import-file irsw $(OBJS) *~ */*~ *.ipk *.o *.a ircp/*.o

include $(BUILD)/Makefile.dpkg_ipkg
include $(BUILD)/Makefile.translation
