PREFIX = /usr/local
PACKAGE = gpe-question
CVSBUILD = yes

ifeq ($(GTK2),yes)
GTKLDFLAGS = `pkg-config --libs gtk+-2.0`
GTKCFLAGS = `pkg-config --cflags gtk+-2.0`
else
GTKLDFLAGS = `gtk-config --libs`
GTKCFLAGS = `gtk-config --cflags` `gdk-pixbuf-config --cflags`
endif

CFLAGS = $(GTKCFLAGS) -Os -Wall -fomit-frame-pointer
CFLAGS += -DPACKAGE=\"$(PACKAGE)\" -DPREFIX=\"$(PREFIX)\" -DPACKAGE_LOCALE_DIR=\"$(PREFIX)/share/locale\"

ifeq ($(CVSBUILD),yes)
CFLAGS += -I../libgpewidget
LDFLAGS += -L../libgpewidget
else
CFLAGS += -I/usr/include/gpe
endif
LDFLAGS += -lgpewidget

all: gpe-question

gpe-question: gpe-question.o
	$(CC) -o $@ $^ $(LDFLAGS) $(GTKLDFLAGS) -lcrypt

install: install-question

install-question: gpe-question
	install -D gpe-question $(DESTDIR)$(PREFIX)/bin/gpe-question
	strip $(DESTDIR)$(PREFIX)/bin/gpe-question

clean:
	rm -f gpe-question.o gpe-question

include ../build/Makefile.dpkg_ipkg
