PREFIX = /usr/local
PACKAGE = gpe-mixer
DEBUG = yes
CVSBUILD = yes
GST = yes
VERSION = 0.37

MEMBERS = gpe-mixer


PACKAGE_CFLAGS = $(GTKCFLAGS) -DUSE_GPE -D_GNU_SOURCE -Wall
ifeq ($(DEBUG),yes)
PACKAGE_CFLAGS += -O2 -g
PACKAGE_LDFLAGS = -g 
else
PAKAGE_CFLAGS += -Os -fomit-frame-pointer
endif
PACKAGE_CPPFLAGS += -DPACKAGE=\"$(PACKAGE)\" -DPREFIX=\"$(PREFIX)\" -DPACKAGE_LOCALE_DIR=\"$(PREFIX)/share/locale\"

ifeq ($(CVSBUILD),yes)
PACKAGE_CFLAGS += -I../libgpewidget
PACKAGE_LDFLAGS += -L../libgpewidget
BUILD = ../build
else
PACKAGE_CFLAGS += -I/usr/include/gpe
BUILD = build
endif
PACKAGE_LDFLAGS += -$(GPELIBS) $(GTKLIBS)

OBJS = $(patsubst %,%.o,$(MEMBERS))
SOURCES = $(patsubst %,%.c,$(MEMBERS))

all: $(PACKAGE)

$(PACKAGE): $(OBJS) $(LIB)
	$(CC) -o $@ $^ $(LDFLAGS) $(PACKAGE_LDFLAGS)

install-program: all $(PACKAGE).desktop
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	install -s $(PACKAGE) $(DESTDIR)$(PREFIX)/bin/$(PACKAGE)
	mkdir -p $(DESTDIR)$(PREFIX)/share/pixmaps
	install -m 644 $(PACKAGE).png $(DESTDIR)$(PREFIX)/share/pixmaps/$(PACKAGE).png
	mkdir -p $(DESTDIR)$(PREFIX)/share/$(PACKAGE)
	install -m 644 pixmaps/*.png $(DESTDIR)$(PREFIX)/share/$(PACKAGE)/
	mkdir -p $(DESTDIR)$(PREFIX)/share/applications
	install -m 644 $(PACKAGE).desktop $(DESTDIR)$(PREFIX)/share/applications

clean:
	rm -f $(PACKAGE) $(OBJS)

include $(BUILD)/Makefile.translation
include $(BUILD)/Makefile.dpkg_ipkg
