PACKAGE = gpe-package
VERSION = 0.1.0
CVSBUILD = yes
DEBUG = no
INSTALL = install
STRIP = strip
PREFIX = /usr/local
DESTDIR = /

LINGUAS = 

EXTRA_DESKTOPS = gpe-package.desktop.in 
HELPFILE = gpe-package.html

ifeq ($(CVSBUILD),yes)
BUILD = ../build
else
BUILD = build
endif

MEMBERS  = main interface packages filesel

ifeq ($(DEBUG),yes)
CFLAGS += -g -DDEBUG
else
CFLAGS += -Os -fomit-frame-pointer
endif

PACKAGE_CFLAGS += -DVERSION=\"$(VERSION)\" -DPREFIX=\"$(PREFIX)\" -D_GNU_SOURCE
PACKAGE_CFLAGS += -DPACKAGE=\"$(PACKAGE)\" -DPACKAGE_LOCALE_DIR=\"$(PREFIX)/share/locale\"
PACKAGE_CFLAGS += -DIPKG_LIB  -I/skiff/local/arm-linux/include/libipkg

GTKCFLAGS = `pkg-config --cflags gtk+-2.0 gdk-pixbuf-2.0 libipkg` 
GTKLDFLAGS = `pkg-config --libs gtk+-2.0 gdk-2.0 gdk-pixbuf-2.0 libipkg`

PACKAGE_CFLAGS += $(GPECFLAGS) $(GTKCFLAGS) $(STANDARD_CFLAGS) 
PACKAGE_LDFLAGS += $(GPELIBS) $(GTKLDFLAGS) $(STANDARD_LDFLAGS)

OBJS = $(patsubst %,%.o,$(MEMBERS))
DEPS = $(patsubst %,%.d,$(MEMBERS))
SOURCES = $(patsubst %,%.c,$(MEMBERS))


PIXMAPS = gpe-package.png local-package-16.png

all: $(PACKAGE) 

$(PACKAGE): $(OBJS)
	$(CC) -g -o $@ $(OBJS) $(LDFLAGS) $(PACKAGE_LDFLAGS)

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
