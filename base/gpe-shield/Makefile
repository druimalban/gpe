PACKAGE = gpe-shield
VERSION = 0.3
NATIVE = yes
CVSBUILD = yes
DEBUG = no
INSTALL = install
PREFIX = /usr/local
DESTDIR = /

LINGUAS = 

EXTRA_DESKTOPS = gpe-shield.desktop.in 
HELPFILE = gpe-shield.html

ifeq ($(CVSBUILD),yes)
GTKLDFLAGS += -L../libgpewidget -L.
CFLAGS += -Wall -I../libgpewidget -I.
BUILD = ../build
else
GTKCFLAGS += -I$(PREFIX)/include/gpe
BUILD = build
endif

MEMBERS  = main interface backend editrule

ifeq ($(DEBUG),yes)
CFLAGS += -g -DDEBUG
else
CFLAGS += -Os -fomit-frame-pointer
endif

CFLAGS += -DVERSION=\"$(VERSION)\" -DPREFIX=\"$(PREFIX)\" -D_GNU_SOURCE
CFLAGS += -DPACKAGE=\"$(PACKAGE)\" -DPACKAGE_LOCALE_DIR=\"$(PREFIX)/share/locale\"

GTKCFLAGS += `pkg-config --cflags gtk+-2.0 gdk-pixbuf-2.0` 
GTKLDFLAGS += `pkg-config --libs gtk+-2.0 gdk-2.0 gdk-pixbuf-2.0`

# setup how to compile
ifeq ($(NATIVE),yes)

STRIP=strip
CC=gcc

CFLAGS += $(GTKCFLAGS) -Wall
LDFLAGS += -lgpewidget $(GTKLDFLAGS)

else # we do cross-compile...

CC=arm-linux-gcc
STRIP=arm-linux-strip

CFLAGS += -march=armv4 -mtune=strongarm
CFLAGS += $(GTKCFLAGS) -Wall 
LDFLAGS += -lgpewidget $(GTKLDFLAGS) -Wall 
LDFLAGS += -lXinerama

endif #native or cross-compile


OBJS = $(patsubst %,%.o,$(MEMBERS))
DEPS = $(patsubst %,%.d,$(MEMBERS))
SOURCES = $(patsubst %,%.c,$(MEMBERS))


PIXMAPS = gpe-shield.png

all: $(PACKAGE) 

$(PACKAGE): $(OBJS)
	$(CC) -g -o $@ $(OBJS) $(LDFLAGS) -lgpewidget $(GTKLDFLAGS)

install-program: $(PACKAGE) all-desktop
	mkdir -p $(DESTDIR)/CONTROL
	sed -e s/VERSION/$(VERSION)/ < familiar/control > $(DESTDIR)/CONTROL/control
	mkdir -p $(DESTDIR)/etc/gpe
	mkdir -p $(DESTDIR)/$(PREFIX)/bin
	$(INSTALL) $(PACKAGE) $(DESTDIR)$(PREFIX)/bin/$(PACKAGE)
	$(STRIP) $(DESTDIR)$(PREFIX)/bin/$(PACKAGE)
	mkdir -p $(DESTDIR)$(PREFIX)/share/pixmaps
	for i in $(PIXMAPS); do install -m 644 -D pixmaps/$$i $(DESTDIR)$(PREFIX)/share/pixmaps/$$i; done
	mkdir -p $(DESTDIR)$(PREFIX)/share/applications
	$(INSTALL) -m 644 *.desktop $(DESTDIR)$(PREFIX)/share/applications
	mkdir -p $(DESTDIR)$(PREFIX)/share/doc/gpe
	$(INSTALL) -m 644 $(HELPFILE) $(DESTDIR)$(PREFIX)/share/doc/gpe/

clean:
	rm -f $(PACKAGE) $(OBJS) *~ */*~ *.ipk *.o

include $(BUILD)/Makefile.dpkg_ipkg
include $(BUILD)/Makefile.translation
