PACKAGE = gpe-lights
PREFIX = /usr
DEBUG = no
LINGUAS = 
VERSION = 0.13
DEBUG = no
CVSBUILD = yes

MEMBERS = gpe-lights

GTKCFLAGS = `pkg-config --cflags gtk+-2.0`
GTKLDFLAGS = `pkg-config --libs gtk+-2.0 gdk-2.0`

PACKAGE_CFLAGS += $(STANDARD_CFLAGS) $(GTKCFLAGS) $(GPECFLAGS)
PACKAGE_CPPFLAGS += $(STANDARD_CPPFLAGS) -DPACKAGE=\"$(PACKAGE)\" -DPREFIX=\"$(PREFIX)\" -DPACKAGE_LOCALE_DIR=\"$(PREFIX)/share/locale\"
PACKAGE_LDFLAGS += $(STANDARD_LDFLAGS) $(GTKLDFLAGS) $(GPELIBS)

ifeq ($(CVSBUILD),yes)
BUILD = ../../base/build
else
BUILD = build
endif

OBJS = $(patsubst %,%.o,$(MEMBERS))
SOURCES = $(patsubst %,%.c,$(MEMBERS))
DEPS = $(patsubst %,%.d,$(MEMBERS))

all: $(PACKAGE)

$(PACKAGE): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS) $(PACKAGE_LDFLAGS)

install-program: all 
	install -s -D $(PACKAGE) $(DESTDIR)$(PREFIX)/bin/$(PACKAGE)
	install -d $(DESTDIR)$(PREFIX)/share/pixmaps
	install -d $(DESTDIR)$(PREFIX)/share/applications
	install -m 644 $(PACKAGE).png $(DESTDIR)$(PREFIX)/share/pixmaps/
	install -m 644 $(PACKAGE).desktop $(DESTDIR)$(PREFIX)/share/applications
	install -d $(DESTDIR)$(PREFIX)/share/$(PACKAGE)
	install -m 644 bulb.png $(DESTDIR)$(PREFIX)/share/$(PACKAGE)
	install -m 644 oses.png $(DESTDIR)$(PREFIX)/share/$(PACKAGE)
	install -m 644 colors.png $(DESTDIR)$(PREFIX)/share/$(PACKAGE)
	install -m 644 face.png $(DESTDIR)$(PREFIX)/share/$(PACKAGE)

clean:
	rm -f $(PACKAGE) $(OBJS) $(DEPS)

include $(BUILD)/Makefile.dpkg_ipkg
include $(BUILD)/Makefile.translation
-include $(DEPS)
