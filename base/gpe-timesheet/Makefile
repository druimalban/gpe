PREFIX = /usr/local
PACKAGE = gpe-timesheet
CVSBUILD = no
VERSION = 0.18

LINGUAS = id pt zh_TW fr ro cs nl ga ru sr sk sv es

PACKAGE_CPPFLAGS += $(STANDARD_CPPFLAGS) -g
PACKAGE_CFLAGS += $(STANDARD_CFLAGS) $(GPECFLAGS) -g
PACKAGE_LDFLAGS += $(STANDARD_LDFLAGS) $(GPELIBS) -lsqlite

MEMBERS = main sql html

DEPS = $(patsubst %,%.d,$(MEMBERS))
OBJS = $(patsubst %,%.o,$(MEMBERS))
SOURCES = $(patsubst %,%.c,$(MEMBERS))

ifeq ($(CVSBUILD),yes)
BUILD = ../build
else
BUILD = build
endif

all: $(PACKAGE)

$(PACKAGE): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS) $(PACKAGE_LDFLAGS)

install-program: all $(PACKAGE).desktop
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	install -s $(PACKAGE) $(DESTDIR)$(PREFIX)/bin/$(PACKAGE)
	mkdir -p $(DESTDIR)$(PREFIX)/share/pixmaps
	install -m 644 $(PACKAGE).png $(DESTDIR)$(PREFIX)/share/pixmaps/$(PACKAGE).png
	mkdir -p $(DESTDIR)$(PREFIX)/share/applications
	install -m 644 $(PACKAGE).desktop $(DESTDIR)$(PREFIX)/share/applications/$(PACKAGE).desktop

clean:
	rm -f $(PACKAGE) $(OBJS) $(DEPS) $(PACKAGE).desktop
	rm -rf debian/gpe-timesheet

include $(BUILD)/Makefile.translation
include $(BUILD)/Makefile.dpkg_ipkg
-include $(DEPS)
