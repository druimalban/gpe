PREFIX = /usr/local
PACKAGE = gpe-dm
CVSBUILD = yes
VERSION = 0.27

ifeq ($(CVSBUILD),yes)
BUILD = ../build
else
BUILD = build
endif

MEMBERS = gpe-dm

CFLAGS = `pkg-config --cflags glib-2.0`
LDFLAGS = `pkg-config --libs glib-2.0`

CPPFLAGS += -DPACKAGE=\"$(PACKAGE)\" -DPREFIX=\"$(PREFIX)\" -DPACKAGE_LOCALE_DIR=\"$(PREFIX)/share/locale\"

OBJS = $(patsubst %,%.o,$(MEMBERS))
DEPS = $(patsubst %,%.d,$(MEMBERS))
SOURCES = $(patsubst %,%.c,$(MEMBERS))

all: gpe-dm

gpe-dm: $(SOURCES)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ $^

install-program: gpe-dm
	install -d $(DESTDIR)$(PREFIX)/bin/
	install -s gpe-dm $(DESTDIR)$(PREFIX)/bin/
	install -d $(DESTDIR)/etc/init.d
	install -d $(DESTDIR)/etc/rc2.d
	install gpe-dm.init $(DESTDIR)/etc/init.d/gpe-dm
	install -d $(DESTDIR)/etc/X11
	install Xsession $(DESTDIR)/etc/X11/Xsession
	install Xinit $(DESTDIR)/etc/X11/Xinit
	install Xserver $(DESTDIR)/etc/X11/Xserver

clean: clean-po
	rm -f gpe-dm gpe-dm.o

include $(BUILD)/Makefile.dpkg_ipkg
include $(BUILD)/Makefile.translation
-include $(DEPS)
