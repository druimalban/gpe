PACKAGE = gpe-julia
VERSION = 0.0.5
DEBUG = no
CVSBUILD = yes
PREFIX = /usr/local

MEMBERS = main

ifeq ($(CVSBUILD),yes)
BUILD = ../../base/build
else
BUILD = build
endif

PACKAGE_CPPFLAGS += $(STANDARD_CPPFLAGS) -DENABLE_NLS
PACKAGE_CPPFLAGS += -DPACKAGE=\"$(PACKAGE)\" -DPREFIX=\"$(PREFIX)\" -DPACKAGE_LOCALE_DIR=\"$(PREFIX)/share/locale\"
PACKAGE_CFLAGS += $(STANDARD_CFLAGS) $(GPECFLAGS)
PACKAGE_LDFLAGS += $(STANDARD_LDFLAGS) $(GPELIBS)

OBJS = $(patsubst %,%.o,$(MEMBERS))
SOURCES = $(patsubst %,%.c,$(MEMBERS))
DEPS = $(patsubst %,%.d,$(MEMBERS))

all: $(PACKAGE)

$(PACKAGE): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS) $(PACKAGE_LDFLAGS)

install-program: all install-mo $(PACKAGE).desktop
	install -D $(PACKAGE) $(DESTDIR)$(PREFIX)/bin/$(PACKAGE)
	install -D -m 644 $(PACKAGE).png $(DESTDIR)$(PREFIX)/share/pixmaps/$(PACKAGE).png
	install -d $(DESTDIR)$(PREFIX)/share/gpe/pixmaps/default/$(PACKAGE)
	install -m 644 pixmaps/zoom_in.png $(DESTDIR)$(PREFIX)/share/gpe/pixmaps/default/$(PACKAGE)/zoom_in.png
	install -m 644 pixmaps/zoom_out.png $(DESTDIR)$(PREFIX)/share/gpe/pixmaps/default/$(PACKAGE)/zoom_out.png
	mkdir -p $(DESTDIR)$(PREFIX)/share/applications
	install -m 644 $(PACKAGE).desktop $(DESTDIR)$(PREFIX)/share/applications/$(PACKAGE).desktop
	strip $(DESTDIR)$(PREFIX)/bin/$(PACKAGE)

clean:
	rm -f $(PACKAGE) $(OBJS) $(DEPS)

include $(BUILD)/Makefile.dpkg_ipkg
include $(BUILD)/Makefile.translation
-include $(DEPS)
