PACKAGE = gpe-mininet
VERSION = 0.3
NATIVE = yes
CVSBUILD = yes
DEBUG = no
GTK2 = yes
INSTALL = install
PREFIX = /usr/
DESTDIR = /

LINGUAS =

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

GTKCFLAGS = `pkg-config --cflags gtk+-2.0 gdk-pixbuf-2.0 libmb` 
GTKLDFLAGS += `pkg-config --libs gtk+-2.0 gdk-2.0` `pkg-config --libs gthread-2.0 libmb`

# setup how to compile
ifeq ($(NATIVE),yes)

STRIP=strip
CC=gcc

CFLAGS +=  -Wall $(GTKCFLAGS)
LDFLAGS += -lgpewidget $(GTKLDFLAGS) 

else # we do cross-compile...

CC=arm-linux-gcc
STRIP=arm-linux-strip

CFLAGS +=  -Wall -mtune=strongarm -march=armv4 $(GTKCFLAGS) 
LDFLAGS += -L/skiff/local/arm-linux/lib -L/skiff/local/arm-linux/lib/X11  
LDFLAGS += -lgpewidget $(GTKLDFLAGS)
LDFLAGS += -lm 
LDFLAGS += -lgmodule-2.0 -ldl -lXinerama
LDFLAGS += -lX11 -lXext -lXrender -lgdk-x11-2.0

endif #native or cross-compile


MEMBERS  = main netlink iwlib

OBJS = $(patsubst %,%.o,$(MEMBERS))
DEPS = $(patsubst %,%.d,$(MEMBERS))
SOURCES = $(patsubst %,%.c,$(MEMBERS))


PIXMAPS = gpe-mininet.png
PIXMAPS += net-on.png
PIXMAPS += net-off.png

all: $(PACKAGE) 

$(PACKAGE): $(OBJS)
	$(CC) -g -o $@ $(OBJS) $(LDFLAGS)

CFLAGS += $(MIMECFLAGS)


install-program: $(PACKAGE) all-desktop
	mkdir -p $(DESTDIR)/$(PREFIX)/bin
	$(INSTALL) $(PACKAGE) $(DESTDIR)$(PREFIX)/bin/
	$(STRIP) $(DESTDIR)$(PREFIX)/bin/$(PACKAGE)
	mkdir -p $(DESTDIR)$(PREFIX)/share/gpe/pixmaps/default
	for i in $(PIXMAPS); do install -m 644 -D pixmaps/$$i $(DESTDIR)$(PREFIX)/share/gpe/pixmaps/default/$$i; done
	mkdir -p $(DESTDIR)$(PREFIX)/share/applications
	$(INSTALL) -m 644 *.desktop $(DESTDIR)$(PREFIX)/share/applications

clean:
	rm -f $(PACKAGE) $(OBJS) *~ */*~ *.ipk *.o

include $(BUILD)/Makefile.dpkg_ipkg
include $(BUILD)/Makefile.translation
