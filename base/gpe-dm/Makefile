PREFIX = /usr/local
PACKAGE = gpe-dm
CVSBUILD = yes
GTK2 = yes
VERSION = 0.21

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
	install -Ds gpe-dm $(DESTDIR)$(PREFIX)/bin/gpe-dm
	mkdir -p $(DESTDIR)/etc/init.d
	mkdir -p $(DESTDIR)/etc/rc2.d
	install gpe-dm.init $(DESTDIR)/etc/init.d/gpe-dm
	ln -sf /etc/init.d/gpe-dm $(DESTDIR)/etc/rc2.d/S99gpe-dm
	install -d $(DESTDIR)/etc/X11
	install Xsession $(DESTDIR)/etc/X11/Xsession
	install Xinit $(DESTDIR)/etc/X11/Xinit
	install Xserver $(DESTDIR)/etc/X11/Xserver

clean: clean-po
	rm -f gpe-login gpe-login.o gpe-dm gpe-dm.o gpe-lock-display gpe-lock-display.o nocursor.o

include $(BUILD)/Makefile.dpkg_ipkg
include $(BUILD)/Makefile.translation
-include $(DEPS)
