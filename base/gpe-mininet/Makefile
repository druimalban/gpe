PACKAGE = gpe-mininet
VERSION = 0.5
NATIVE = yes
CVSBUILD = yes
DEBUG = no
INSTALL = install
PREFIX = /usr/local
DESTDIR = /

LINGUAS =

ifeq ($(CVSBUILD),yes)
PACKAGE_LDFLAGS += -L../libgpewidget -L.
PACKAGE_CFLAGS += -Wall -I../libgpewidget 
BUILD = ../build
else
PACKAGE_CFLAGS += -I$(PREFIX)/include/gpe
BUILD = build
endif

ifeq ($(DEBUG),yes)
PACKAGE_CFLAGS += -g -DDEBUG
else
PACKAGE_CFLAGS += -Os -fomit-frame-pointer
endif

PACKAGE_CFLAGS += -DVERSION=\"$(VERSION)\" -DPREFIX=\"$(PREFIX)\" -D_GNU_SOURCE -DPACKAGE=\"$(PACKAGE)\" 
PACKAGE_CFLAGS += -DPACKAGE_LOCALE_DIR=\"$(PREFIX)/share/locale\"

PACKAGE_CPPFLAGS += -I.

PACKAGE_CFLAGS +=  -Wall $(GPECFLAGS) `pkg-config --cflags gdk-pixbuf-2.0 libmb`
PACKAGE_LDFLAGS += -lgpewidget $(GPELDFLAGS) `pkg-config --libs gdk-pixbuf-2.0 libmb`

MEMBERS  = main netlink iwlib

OBJS = $(patsubst %,%.o,$(MEMBERS))
DEPS = $(patsubst %,%.d,$(MEMBERS))
SOURCES = $(patsubst %,%.c,$(MEMBERS))


PIXMAPS = gpe-mininet.png
PIXMAPS += net-on.png
PIXMAPS += net-off.png

all: $(PACKAGE) 

$(PACKAGE): $(OBJS)
	$(CC) -g -o $@ $(OBJS) $(LDFLAGS) $(PACKAGE_LDFLAGS)


install-program: $(PACKAGE) all-desktop
	mkdir -p $(DESTDIR)/$(PREFIX)/bin
	$(INSTALL) $(PACKAGE) $(DESTDIR)$(PREFIX)/bin/
	mkdir -p $(DESTDIR)$(PREFIX)/share/gpe/pixmaps/default
	for i in $(PIXMAPS); do install -m 644 -D pixmaps/$$i $(DESTDIR)$(PREFIX)/share/gpe/pixmaps/default/$$i; done
	mkdir -p $(DESTDIR)$(PREFIX)/share/applications
	$(INSTALL) -m 644 *.desktop $(DESTDIR)$(PREFIX)/share/applications

clean:
	rm -f $(PACKAGE) $(OBJS) *~ */*~ *.ipk *.o

include $(BUILD)/Makefile.dpkg_ipkg
include $(BUILD)/Makefile.translation
