PACKAGE = gpe-aerial
VERSION = 0.2.10
CVSBUILD = yes
DEBUG = no
INSTALL = install
STRIP=strip
PREFIX = /usr/
DESTDIR = /

LINGUAS = de sv ro cs nl pt ru fr sr

ifeq ($(CVSBUILD),yes)
BUILD = ../build
else
BUILD = build
endif

ifeq ($(DEBUG),yes)
PACKAGE_CFLAGS += -g -DDEBUG
else
PACKAGE_CFLAGS += -Os -fomit-frame-pointer
endif

PACKAGE_CFLAGS += -Wall -DVERSION=\"$(VERSION)\" -DPREFIX=\"$(PREFIX)\" -D_GNU_SOURCE -DPACKAGE=\"$(PACKAGE)\" 
PACKAGE_CFLAGS += -DPACKAGE_LOCALE_DIR=\"$(PREFIX)/share/locale\"

PACKAGE_CPPFLAGS += -I.

GTKCFLAGS = `pkg-config --cflags gtk+-2.0 gdk-pixbuf-2.0` 
GTKLDFLAGS += `pkg-config --libs gtk+-2.0 gdk-2.0` `pkg-config --libs gthread-2.0`

PACKAGE_CFLAGS +=  -Wall $(GPECFLAGS) $(GTKCFLAGS) 
PACKAGE_LDFLAGS += $(GPELIBS) $(GTKLDFLAGS) -lsqlite


MEMBERS  = main netdb netedit

OBJS = $(patsubst %,%.o,$(MEMBERS))
DEPS = $(patsubst %,%.d,$(MEMBERS))
SOURCES = $(patsubst %,%.c,$(MEMBERS))


PIXMAPS = gpe-aerial.png
PIXMAPS += scan-on-16.png
PIXMAPS += scan-off-16.png

all: $(PACKAGE) 

$(PACKAGE): $(OBJS)
	$(CC) -g -o $@ $(OBJS) $(LDFLAGS) $(PACKAGE_LDFLAGS)


install-program: $(PACKAGE) all-desktop
	mkdir -p $(DESTDIR)/$(PREFIX)/bin
	$(INSTALL) $(PACKAGE) $(DESTDIR)$(PREFIX)/bin/
	$(STRIP) $(DESTDIR)$(PREFIX)/bin/$(PACKAGE)
	mkdir -p $(DESTDIR)$(PREFIX)/share/pixmaps/
	for i in $(PIXMAPS); do install -m 644 -D pixmaps/$$i $(DESTDIR)$(PREFIX)/share/pixmaps/$$i; done
	mkdir -p $(DESTDIR)$(PREFIX)/share/applications
	$(INSTALL) -m 644 *.desktop $(DESTDIR)$(PREFIX)/share/applications

clean:
	rm -f gpe-aerial $(OBJS) *~ */*~ *.ipk *.o

include $(BUILD)/Makefile.dpkg_ipkg
include $(BUILD)/Makefile.translation
