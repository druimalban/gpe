PREFIX = /usr/local
PACKAGE = libgpewidget
DEBUG = yes
VERSION = 0.96
CVSBUILD = yes
CAIRO = yes

IN_LIBGPEWIDGET = yes

LINGUAS = ro de id pt fr ga wa sv nl cs es

MEMBERS = gtkdatecombo errorbox init pixmaps smallbox render dirbrowser picturebutton what stylus screensize question gtksimplemenu spacing tray translabel popup popup_menu gpetimesel gpeiconlistitem gpeclockface gpetooltips2 gpe-iconlist gpewindowlist gpehelp gpeiconlistview gpewindowbits infoprint

SONAME = libgpewidget.so.1
BINPACKAGE = libgpewidget1

ifeq ($(CVSBUILD),yes)
BUILD = ../build
else
BUILD = build
endif

PACKAGE_CPPFLAGS = $(STANDARD_CPPFLAGS)
PACKAGE_CFLAGS = $(STANDARD_CFLAGS) $(GTKCFLAGS)
PACKAGE_LDFLAGS = $(STANDARD_LDFLAGS) $(GTKLIBS)

PACKAGE_CFLAGS += -fPIC
PACKAGE_CPPFLAGS += -I./gpe -I.

ifeq ($(CAIRO),yes)
PACKAGE_CPPFLAGS += -DHAVE_CAIRO
PACKAGE_CFLAGS += $(shell pkg-config --cflags cairo)
PACKAGE_LDFLAGS += $(shell pkg-config --libs cairo)
endif

HEADERS += init.h render.h errorbox.h smallbox.h pixmaps.h gtkdatecombo.h dirbrowser.h stylus.h picturebutton.h spacing.h translabel.h question.h windows.h gpe-iconlist.h gtksimplemenu.h gpewindowlist.h tray.h popup.h gpeiconlistview.h gpeclockface.h gpeiconlistitem.h infoprint.h

OBJS = $(patsubst %,%.o,$(MEMBERS))
DEPS = $(patsubst %,%.d,$(MEMBERS))
SOURCES = $(patsubst %,%.c,$(MEMBERS))

all: libgpewidget.so $(PACKAGE).pc $(PACKAGE)-uninstalled.pc

libgpewidget.so: $(SONAME)
	ln -sf $^ $@

$(SONAME): $(OBJS)
	$(CC) -nostartfiles -shared -o $@ $^ -Wl,-soname -Wl,$(SONAME) $(LDFLAGS) $(PACKAGE_LDFLAGS)

gpe-iconlist.c: gpeiconlist.gob
	gob2 --no-private-header gpeiconlist.gob
	mv -f gpe-iconlist.h gpe/

libgpewidget.a: $(OBJS)
	ar cq $@ $^

install-program: $(SONAME)
	mkdir -p $(DESTDIR)$(PREFIX)/lib/
	install -s $(SONAME) $(DESTDIR)$(PREFIX)/lib/
	install -d $(DESTDIR)$(PREFIX)/share/libgpewidget/
	install -m 644 pixmaps/clock.png $(DESTDIR)$(PREFIX)/share/libgpewidget/clock.png
	install -m 644 pixmaps/clock24.png $(DESTDIR)$(PREFIX)/share/libgpewidget/clock24.png
	install -m 644 pixmaps/day-night-wheel.png $(DESTDIR)$(PREFIX)/share/libgpewidget/day-night-wheel.png

install-devel: $(PACKAGE).pc
	for i in $(HEADERS); do install -m 644 -D gpe/$$i $(DESTDIR)$(PREFIX)/include/gpe/$$i; done
	ln -sf $(SONAME) $(DESTDIR)$(PREFIX)/lib/libgpewidget.so
	install -d $(DESTDIR)$(PREFIX)/lib/pkgconfig
	install -m 644 $(PACKAGE).pc $(DESTDIR)$(PREFIX)/lib/pkgconfig/

clean: clean-po
	rm -f $(OBJS) $(SONAME) libgpewidget.so $(DEPS) $(PACKAGE).pc $(PACKAGE)-uninstalled.pc

ipkg: all

include $(BUILD)/Makefile.translation
include $(BUILD)/Makefile.dpkg_ipkg
-include $(DEPS)
