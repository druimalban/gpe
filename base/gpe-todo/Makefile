PREFIX = /usr/local
PACKAGE = gpe-todo
GTK2 = yes
CVSBUILD = yes
VERSION = 0.47
DEBUG = no

LINGUAS = de id pt zh_TW fr ro cs nl ru sr sk sv wa es

ifeq ($(CVSBUILD),yes)
PACKAGE_CPPFLAGS += -I../libdisplaymigration -I../libtododb -I../libgpepimc
PACKAGE_LDFLAGS += -L../libdisplaymigration -L../libtododb -L../libgpepimc
endif
PACKAGE_CPPFLAGS += $(STANDARD_CPPFLAGS)
PACKAGE_CFLAGS += $(STANDARD_CFLAGS) $(GPECFLAGS)
PACKAGE_LDFLAGS += $(STANDARD_LDFLAGS) $(GPELIBS) -ltododb -ldisplaymigration -lgpepimc

MEMBERS = main todo-ui list-ui

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
	$(CC) -o $@ $^ $(LDFLAGS) $(PACKAGE_LDFLAGS)

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
	mkdir -p $(DESTDIR)$(PREFIX)/share/$(PACKAGE)
	install -m 644 tick-box.png $(DESTDIR)$(PREFIX)/share/$(PACKAGE)/tick-box.png
	install -m 644 notick-box.png $(DESTDIR)$(PREFIX)/share/$(PACKAGE)/notick-box.png

include $(BUILD)/Makefile.translation
include $(BUILD)/Makefile.dpkg_ipkg
-include $(DEPS)
