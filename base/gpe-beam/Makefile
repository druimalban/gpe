PACKAGE = gpe-beam
VERSION = 0.2.2
NATIVE = yes
CVSBUILD = yes
DEBUG = no
GTK2 = yes
INSTALL = install
PREFIX = /usr
DESTDIR = /
CONTROL = control1

LINGUAS = cs de sv ro nl pt ru fr sr

ifeq ($(CVSBUILD),yes)
LDFLAGS += -L../libgpewidget -L.
CFLAGS += -Wall -I../libgpewidget 
BUILD = ../build
else
CFLAGS += -I$(PREFIX)/include/gpe
BUILD = build
endif

ifeq ($(DEBUG),yes)
CFLAGS += -g -DDEBUG -DIRCP_DEBUG=9
else
CFLAGS += -Os -fomit-frame-pointer 
endif

CFLAGS += -DVERSION=\"$(VERSION)\" -DPREFIX=\"$(PREFIX)\" -D_GNU_SOURCE -DPACKAGE=\"$(PACKAGE)\" 
CFLAGS += -DPACKAGE_LOCALE_DIR=\"$(PREFIX)/share/locale\"

CPPFLAGS += -I.

GTKCFLAGS = `pkg-config --cflags gtk+-2.0` 
GTKLDFLAGS += `pkg-config --libs gtk+-2.0` `pkg-config --libs gthread-2.0`

MIMECFLAGS =`pkg-config --cflags mimedir-1.0` -I/usr/local/include/mimedir-1.0 -I../libgpevtype
MIMELDFLAGS = $(GTKLDFLAGS) -lsqlite -lmimedir -L../libgpevtype -lgpevtype

libircp_a_OBJECTS =  ircp/ircp_client.o ircp/ircp_server.o ircp/ircp_io.o ircp/dirtraverse.o
IRFLAGS = -lopenobex libircp.a

# setup how to compile
ifeq ($(NATIVE),yes)

STRIP=strip
CC=gcc
AR = ar
RANLIB = ranlib

CFLAGS +=  -Wall $(GTKCFLAGS)
LDFLAGS += -lgpewidget $(GTKLDFLAGS)

else # we do cross-compile...

CC=arm-linux-gcc
STRIP=arm-linux-strip
AR = arm-linux-ar
RANLIB = arm-linux-ranlib

CFLAGS +=  -Wall $(GTKCFLAGS)
LDFLAGS += -L/skiff/local/arm-linux/lib -L/skiff/local/arm-linux/lib/X11  
LDFLAGS += -lgpewidget $(GTKLDFLAGS)
LDFLAGS += -lm 
LDFLAGS += -lgmodule-2.0 -ldl -lXinerama
LDFLAGS += -lX11 -lXext -lXrender -lgdk-x11-2.0

endif #native or cross-compile


MEMBERS  = main filesel

OBJS = $(patsubst %,%.o,$(MEMBERS))
DEPS = $(patsubst %,%.d,$(MEMBERS))
SOURCES = $(patsubst %,%.c,$(MEMBERS))


PIXMAPS = irda-16.png irda.png irda-on-16.png

all: $(PACKAGE) vcard-export vcard-import vcard-import-file irsw vcal-export

$(PACKAGE): libircp.a $(OBJS)
	$(CC) -g -o $@ $(OBJS) $(LDFLAGS) $(IRFLAGS)

irsw: irsw.o irsw.o
	$(CC) -o $@ $^ $(LDFLAGS)

CFLAGS += $(MIMECFLAGS)

vcal-export: vcal-export.o
	$(CC) -o $@ $^ $(LDFLAGS) $(MIMELDFLAGS)

vcard-export: vcard-export.o
	$(CC) -o $@ $^ $(LDFLAGS) $(MIMELDFLAGS)

vcard-import: vcard-import.o
	$(CC) -o $@ $^ $(LDFLAGS) $(MIMELDFLAGS)

vcard-import-file: import-vcard-file.o vcard.o
	$(CC) -o $@ $^ $(LDFLAGS) $(MIMELDFLAGS)

libircp.a: $(libircp_a_OBJECTS) $(libircp_a_DEPENDENCIES)
	-rm -f libircp.a
	$(AR) cru libircp.a $(libircp_a_OBJECTS) $(libircp_a_LIBADD)
	$(RANLIB) libircp.a

install-program: all all-desktop
	mkdir -p $(DESTDIR)/CONTROL
	sed -e s/VERSION/$(VERSION)/ <familiar/$(CONTROL) > $(DESTDIR)/CONTROL/control
	mkdir -p $(DESTDIR)/$(PREFIX)/bin
	$(INSTALL) $(PACKAGE) $(DESTDIR)$(PREFIX)/bin/
	$(INSTALL) irsw $(DESTDIR)$(PREFIX)/bin/
	$(INSTALL) vcard-export $(DESTDIR)$(PREFIX)/bin/
	$(INSTALL) vcard-import $(DESTDIR)$(PREFIX)/bin/
	$(INSTALL) vcard-import-file $(DESTDIR)$(PREFIX)/bin/
	$(STRIP) $(DESTDIR)$(PREFIX)/bin/*
	mkdir -p $(DESTDIR)$(PREFIX)/share/pixmaps
	for i in $(PIXMAPS); do install -m 644 -D pixmaps/$$i $(DESTDIR)$(PREFIX)/share/pixmaps/$$i; done
	mkdir -p $(DESTDIR)$(PREFIX)/share/applications
	$(INSTALL) -m 644 *.desktop $(DESTDIR)$(PREFIX)/share/applications

clean:
	rm -f $(PACKAGE)  vcard-export vcard-import vcard-import-file  $(OBJS) *~ */*~ *.ipk *.o *.a ircp/*.o

include $(BUILD)/Makefile.dpkg_ipkg
include $(BUILD)/Makefile.translation
