PREFIX = /usr/local
PACKAGE = gpe-edit
DEBUG = no
CVSBUILD = yes
VERSION = 0.20

MEMBERS = main
#FIXME: add: cs ru sr ('de' is not on TP?)
LINGUAS = id pt de zh_TW fr ro wa sv nl sk

CPPFLAGS += $(STANDARD_CPPFLAGS)
CFLAGS += $(STANDARD_CFLAGS) $(GPECFLAGS)
LDFLAGS += $(STANDARD_LDFLAGS) $(GPELIBS)

ifeq ($(CVSBUILD),yes)
CFLAGS += -I../libdisplaymigration
LDFLAGS += -L../libdisplaymigration
endif
LDFLAGS += -ldisplaymigration

DEPS = $(patsubst %,%.d,$(MEMBERS))
OBJS = $(patsubst %,%.o,$(MEMBERS))
SOURCES = $(patsubst %,%.c,$(MEMBERS))

ifeq ($(CVSBUILD),yes)
BUILD = ../build
else
BUILD = build
endif

all: gpe-edit all-mo

gpe-edit: $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

install-program: all gpe-edit.desktop
	install -d $(DESTDIR)$(PREFIX)/bin
	install -s gpe-edit $(DESTDIR)$(PREFIX)/bin/gpe-edit
	install -D -m 644 gpe-edit.png $(DESTDIR)$(PREFIX)/share/pixmaps/gpe-edit.png
	install -D -m 644 gpe-edit.mime $(DESTDIR)/etc/mime-handlers/gpe-edit.mime
	mkdir -p $(DESTDIR)$(PREFIX)/share/applications
	install -m 644 $(PACKAGE).desktop $(DESTDIR)$(PREFIX)/share/applications/

clean: clean-po
	rm -f gpe-edit $(OBJS) $(DEPS) $(PACKAGE).desktop

include $(BUILD)/Makefile.translation
include $(BUILD)/Makefile.dpkg_ipkg
-include $(DEPS)
