PREFIX = /usr/local
PACKAGE = libgpewidget
DEBUG = no
VERSION = 0.68
CVSBUILD = yes

LINGUAS = ro de id pt fr ga wa sv nl

MEMBERS = gtkdatecombo errorbox init pixmaps smallbox render dirbrowser picturebutton what stylus screensize question gtksimplemenu spacing tray tap translabel popup windows popup_menu gpetimesel gpeiconlistitem gpeclockface gpetooltips2 gpe-iconlist

SONAME = libgpewidget.so.1
BINPACKAGE = libgpewidget1

ifeq ($(CVSBUILD),yes)
BUILD = ../build
else
BUILD = build
endif

$(eval CFLAGS = $(shell pkg-config --cflags gtk+-2.0))
$(eval LDFLAGS = $(shell pkg-config --libs gtk+-2.0))

CFLAGS += -DPREFIX=\"$(PREFIX)\" -DPACKAGE=\"$(PACKAGE)\" -D_GNU_SOURCE -Wall -fPIC
ifeq ($(DEBUG),yes)
CFLAGS += -O2 -g
LDFLAGS += -g
else
CFLAGS += -Os -fomit-frame-pointer
endif
CFLAGS += -MD

CPPFLAGS += -I./gpe

HEADERS += init.h render.h errorbox.h smallbox.h pixmaps.h gtkdatecombo.h dirbrowser.h stylus.h picturebutton.h spacing.h translabel.h question.h windows.h gpe-iconlist.h gtksimplemenu.h

OBJS = $(patsubst %,%.o,$(MEMBERS))
DEPS = $(patsubst %,%.d,$(MEMBERS))
SOURCES = $(patsubst %,%.c,$(MEMBERS))

all: libgpewidget.so $(PACKAGE).pc $(PACKAGE)-uninstalled.pc

libgpewidget.so: $(SONAME)
	ln -sf $^ $@

$(SONAME): $(OBJS)
	$(CC) -nostartfiles -shared -o $@ $^ -Wl,-soname -Wl,$(SONAME) $(LDFLAGS)

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
	install -d $(PREFIX)/lib/pkg-config
	install -m 644 $(PACKAGE).pc $(PREFIX)/lib/pkg-config/

clean: clean-po
	rm -f $(OBJS) $(SONAME) libgpewidget.so $(DEPS)

include $(BUILD)/Makefile.translation
include $(BUILD)/Makefile.dpkg_ipkg
-include $(DEPS)
