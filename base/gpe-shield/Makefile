PACKAGE = gpe-shield
VERSION = 0.7
NATIVE = yes
CVSBUILD = yes
DEBUG = no
INSTALL = install
STRIP = strip
PREFIX = /usr/local
DESTDIR = /
MACHINE = other

LINGUAS = 


EXTRA_DESKTOPS = gpe-shield.desktop.in 
HELPFILE = gpe-shield.html

ifeq ($(CVSBUILD),yes)
PACKAGE_LDFLAGS += -L../libgpewidget -L.
PACKAGE_CFLAGS += -I../libgpewidget -I.
BUILD = ../build
else
BUILD = build
endif

PACKAGE_CPPFLAGS = $(STANDARD_CPPFLAGS)
PACKAGE_CFLAGS += $(STANDARD_CFLAGS) $(GTKCFLAGS) $(GPECFLAGS)
PACKAGE_LDFLAGS += $(STANDARD_LDFLAGS) $(GPELIBS) $(GTKLIBS)


MEMBERS  = main interface backend editrule

ifeq ($(DEBUG),yes)
PACKAGE_CFLAGS += -g -DDEBUG
else
PACKAGE_CFLAGS += -Os -fomit-frame-pointer
endif

PACKAGE_CFLAGS += -DVERSION=\"$(VERSION)\" -DPREFIX=\"$(PREFIX)\" -D_GNU_SOURCE
PACKAGE_CFLAGS += -DPACKAGE=\"$(PACKAGE)\" -DPACKAGE_LOCALE_DIR=\"$(PREFIX)/share/locale\"

OBJS = $(patsubst %,%.o,$(MEMBERS))
DEPS = $(patsubst %,%.d,$(MEMBERS))
SOURCES = $(patsubst %,%.c,$(MEMBERS))


PIXMAPS = gpe-shield.png

all: $(PACKAGE) 

$(PACKAGE): $(OBJS)
	$(CC) -g -o $@ $(OBJS) $(LDFLAGS) $(PACKAGE_LDFLAGS)

install-program: $(PACKAGE) all-desktop
	mkdir -p $(DESTDIR)/CONTROL
	sed -e s/VERSION/$(VERSION)/ < familiar/control > $(DESTDIR)/CONTROL/control
	$(INSTALL) -m 755 familiar/postinst $(DESTDIR)/CONTROL/postinst
	mkdir -p $(DESTDIR)/etc/gpe
	touch $(DESTDIR)/etc/gpe/gpe-shield-load
	mkdir -p $(DESTDIR)/$(PREFIX)/bin
	$(INSTALL) -m 4755 $(PACKAGE) $(DESTDIR)$(PREFIX)/bin/$(PACKAGE)
	$(STRIP) $(DESTDIR)$(PREFIX)/bin/$(PACKAGE)
	mkdir -p $(DESTDIR)/etc/init.d/
	$(INSTALL) -m 755 ipshield $(DESTDIR)/etc/init.d/ipshield
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
