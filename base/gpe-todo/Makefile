PREFIX = /usr/local
PACKAGE = gpe-todo
GTK2 = yes
CVSBUILD = yes
VERSION = 0.38
DEBUG = no

LINGUAS = de id pt zh_TW fr ro

GTKCFLAGS = `pkg-config --cflags gtk+-2.0`
GTKLDFLAGS += `pkg-config --libs gtk+-2.0`

CPPFLAGS += -D_GNU_SOURCE

CFLAGS = $(GTKCFLAGS) -g -I../libgpewidget -Wall -I../libdisplaymigration -I../libtododb
CFLAGS += -DPACKAGE=\"$(PACKAGE)\" -DPREFIX=\"$(PREFIX)\" -DPACKAGE_LOCALE_DIR=\"$(PREFIX)/share/locale\"
CFLAGS += -MD

ifeq ($(DEBUG),yes)
CFLAGS += -O2 -g
LDFLAGS = -g
else
CFLAGS += -Os -fomit-frame-pointer
endif

MEMBERS = main todo-ui list-ui configure

DEPS = $(patsubst %,%.d,$(MEMBERS))
OBJS = $(patsubst %,%.o,$(MEMBERS))
SOURCES = $(patsubst %,%.c,$(MEMBERS))

ifeq ($(CVSBUILD),yes)
BUILD = ../build
else
BUILD = build
endif

all: $(PACKAGE) all-mo

$(PACKAGE): $(OBJS)
	$(CC) -o $@ $^ -L../libgpewidget -lgpewidget $(GTKLDFLAGS) -g -lsqlite -L../libdisplaymigration -ldisplaymigration -L../libtododb -ltododb

clean: clean-po
	rm -f $(PACKAGE) $(OBJS) $(DEPS) $(PACKAGE).desktop

all: $(PACKAGE)

install-program: all $(PACKAGE).desktop
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	install -s $(PACKAGE) $(DESTDIR)$(PREFIX)/bin/$(PACKAGE)
	mkdir -p $(DESTDIR)$(PREFIX)/share/pixmaps
	install -m 644 $(PACKAGE).png $(DESTDIR)$(PREFIX)/share/pixmaps/$(PACKAGE).png
	mkdir -p $(DESTDIR)$(PREFIX)/share/applications
	install -m 644 $(PACKAGE).desktop $(DESTDIR)$(PREFIX)/share/applications/$(PACKAGE).desktop

include $(BUILD)/Makefile.translation
include $(BUILD)/Makefile.dpkg_ipkg
-include $(DEPS)
