PACKAGE = gpe-beam
VERSION = 0.2.0
NATIVE = yes
CVSBUILD = yes
DEBUG = no
GTK2 = yes
INSTALL = install
PREFIX = /usr
DESTDIR = /

ifeq ($(CVSBUILD),yes)
LDFLAGS += -L../libgpewidget -L.
CFLAGS += -Wall -I../libgpewidget 
BUILD = ../build
else
CFLAGS += -I$(PREFIX)/include/gpe
BUILD = build
endif

ifeq ($(DEBUG),yes)
CFLAGS += -g -DDEBUG
else
CFLAGS += -Os -fomit-frame-pointer
endif

CFLAGS += -DVERSION=\"$(VERSION)\" -DPREFIX=\"$(PREFIX)\" -D_GNU_SOURCE -DPACKAGE=\"$(PACKAGE)\" 
CFLAGS += -DPACKAGE_LOCALE_DIR=\"$(PREFIX)/share/locale\"

CPPFLAGS += -I.

GTKCFLAGS = `pkg-config --cflags gtk+-2.0 gdk-pixbuf-2.0` 
GTKLDFLAGS += `pkg-config --libs gtk+-2.0 gdk-2.0` `pkg-config --libs gthread-2.0`

MIMECFLAGS =`pkg-config --cflags mimedir-1.0` -I/usr/local/include/mimedir-1.0
MIMELDFLAGS = $(GTKLDFLAGS) -lsqlite -lmimedir 

# setup how to compile
ifeq ($(NATIVE),yes)

STRIP=strip
CC=gcc

CFLAGS +=  -Wall
LDFLAGS += -lgpewidget $(GTKLDFLAGS)

else # we do cross-compile...

CC=arm-linux-gcc
STRIP=arm-linux-strip

CFLAGS +=  -Wall $(GTKCFLAGS)
LDFLAGS += -L/skiff/local/arm-linux/lib -L/skiff/local/arm-linux/lib/X11  
LDFLAGS += -lgpewidget $(GTKLDFLAGS)
LDFLAGS += -lm 
LDFLAGS += -lgmodule-2.0 -ldl -lXinerama
LDFLAGS += -lX11 -lXext -lXrender -lgdk-x11-2.0

endif #native or cross-compile


MEMBERS  = main 

OBJS = $(patsubst %,%.o,$(MEMBERS))
DEPS = $(patsubst %,%.d,$(MEMBERS))
SOURCES = $(patsubst %,%.c,$(MEMBERS))


PIXMAPS = irda-16.png irda.png

all: $(PACKAGE) vcard-export vcard-import vcard-import-file

$(PACKAGE): $(OBJS)
	$(CC) -g -o $@ $(OBJS) $(LDFLAGS)

CFLAGS += $(MIMECFLAGS)

vcard-export: send-vcard.o vcard.o
	$(CC) -o $@ $^ $(LDFLAGS) $(MIMELDFLAGS)

vcard-import: import-vcard.o vcard.o
	$(CC) -o $@ $^ $(LDFLAGS) $(MIMELDFLAGS)

vcard-import-file: import-vcard-file.o vcard.o
	$(CC) -o $@ $^ $(LDFLAGS) $(MIMELDFLAGS)


install-program: $(PACKAGE)
	sed -e s/VERSION/$(VERSION)/ <$(CONTROL).pre > $(CONTROL)
	mkdir -p $(DESTDIR)/$(PREFIX)/bin
	$(INSTALL) $(PACKAGE) $(DESTDIR)$(PREFIX)/bin/
	$(STRIP) $(DESTDIR)$(PREFIX)/bin/$(PACKAGE)
	mkdir -p $(DESTDIR)$(PREFIX)/share/pixmaps
	for i in $(PIXMAPS); do install -m 644 -D pixmaps/$$i $(DESTDIR)$(PREFIX)/share/pixmaps/$$i; done
	mkdir -p $(DESTDIR)$(PREFIX)/share/applications
	$(INSTALL) -m 644 *.desktop $(DESTDIR)$(PREFIX)/share/applications

clean:
	rm -f $(PACKAGE)  vcard-export vcard-import vcard-import-file  $(OBJS) *~ */*~ *.ipk *.o

include $(BUILD)/Makefile.dpkg_ipkg
include $(BUILD)/Makefile.translation
