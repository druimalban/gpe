PREFIX = /usr/local
PACKAGE = libgpewidget
DEBUG = no
GTK2 = yes
VERSION = 0.46
CVSBUILD = yes

GTKRC = gtkrc

LINGUAS = 

MEMBERS = gtkdatecombo errorbox init pixmaps smallbox render dirbrowser picturebutton what stylus gtkdatesel screensize question gtksimplemenu spacing tray tap

ifeq ($(GTK2),yes)
MEMBERS += gpetooltips2 gpe-iconlist
SONAME = libgpewidget.so.1
CONTROL = control1
else
MEMBERS += gpetooltips gtkgpepixmap about
SONAME = libgpewidget.so.0
CONTROL = control
endif

ifeq ($(CVSBUILD),yes)
BUILD = ../build
else
BUILD = build
endif

ifeq ($(GTK2),yes)
CFLAGS = `pkg-config --cflags gtk+-2.0`
LDFLAGS = `pkg-config --libs gtk+-2.0`
else
CFLAGS = `gdk-pixbuf-config --cflags` `gtk-config --cflags`
LDFLAGS =`gtk-config --libs` `gdk-pixbuf-config --libs` 
endif
CFLAGS += -DPREFIX=\"$(PREFIX)\" -DPACKAGE=\"$(PACKAGE)\" -D_GNU_SOURCE -Wall -fPIC
ifeq ($(DEBUG),yes)
CFLAGS += -O2 -g
LDFLAGS += -g
else
CFLAGS += -Os -fomit-frame-pointer
endif
CFLAGS += -MD

CPPFLAGS += -I./gpe

HEADERS = init.h render.h errorbox.h smallbox.h pixmaps.h gtkdatecombo.h gtkdatesel.h dirbrowser.h stylus.h picturebutton.h spacing.h

OBJS = $(patsubst %,%.o,$(MEMBERS))
DEPS = $(patsubst %,%.d,$(MEMBERS))
SOURCES = $(patsubst %,%.c,$(MEMBERS))

all: libgpewidget.so

libgpewidget.so: $(SONAME)
	ln -sf $^ $@

$(SONAME): $(OBJS)
	$(CC) -nostartfiles -shared -o $@ $^ -Wl,-soname -Wl,$(SONAME) $(LDFLAGS)

gpe-iconlist.c: gpeiconlist.gob
	gob2 --no-private-header gpeiconlist.gob
	mv -f gpe-iconlist.h gpe/

libgpewidget.a: $(OBJS)
	ar cq $@ $^

install: $(SONAME) install-mo
	install -D $(SONAME) $(DESTDIR)$(PREFIX)/lib/$(SONAME)
	strip $(DESTDIR)$(PREFIX)/lib/$(SONAME)
ifneq ($(GTK2),yes)
	install -D $(GTKRC) $(DESTDIR)$(PREFIX)/share/gpe/$(GTKRC)
endif

install-devel:
	for i in $(HEADERS); do install -m 644 -D gpe/$$i $(DESTDIR)$(PREFIX)/include/gpe/$$i; done
	ln -s $(SONAME) $(DESTDIR)$(PREFIX)/lib/libgpewidget.so

clean: clean-po
	rm -f $(OBJS) $(SONAME) libgpewidget.so $(DEPS)

include $(BUILD)/Makefile.translation
include $(BUILD)/Makefile.dpkg_ipkg

-include $(DEPS)
