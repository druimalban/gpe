PREFIX = /usr/local
PACKAGE = gpe-gallery
DEBUG = yes
CVSBUILD = yes
GTK2 = yes

MEMBERS = main image_tools

ifeq ($(GTK2),yes)
CFLAGS = `pkg-config --cflags gtk+-2.0`
CONTROL = control1
else
CFLAGS = `gdk-pixbuf-config --cflags` `gtk-config --cflags` 
CONTROL = control
endif

CFLAGS += -D_GNU_SOURCE -Wall
ifeq ($(DEBUG),yes)
CFLAGS += -O2 -g
LDFLAGS = -g #-lefence
else
CFLAGS += -Os -fomit-frame-pointer
endif

ifeq ($(CVSBUILD),yes)
CFLAGS += -I../libgpewidget
LDFLAGS += -L../libgpewidget
else
CFLAGS += -I/usr/include/gpe
endif
LDFLAGS += -lgpewidget

ifeq ($(GTK2),yes)
LDFLAGS += `pkg-config --libs gtk+-2.0`
else
LDFLAGS += `gtk-config --libs` `gdk-pixbuf-config --libs`
endif

CFLAGS += -DPACKAGE=\"$(PACKAGE)\" -DPREFIX=\"$(PREFIX)\" -DPACKAGE_LOCALE_DIR=\"$(PREFIX)/share/locale\"

OBJS = $(patsubst %,%.o,$(MEMBERS))
SOURCES = $(patsubst %,%.c,$(MEMBERS))

all: gpe-gallery

gpe-gallery: $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

install: all
	install -D gpe-gallery $(DESTDIR)$(PREFIX)/bin/gpe-gallery
	install -D -m 644 gpe-gallery.png $(DESTDIR)$(PREFIX)/share/pixmaps/gpe-gallery.png
	install -D -m 644 gpe-gallery.menu $(DESTDIR)$(PREFIX)/lib/menu/gpe-gallery
	install -D -m 644 gpe-gallery.mime $(DESTDIR)/etc/mime-handlers/gpe-gallery.mime
	install -d $(DESTDIR)$(PREFIX)/share/gpe/pixmaps/default/gallery
	install -m 644 pixmaps/slideshow.png $(DESTDIR)$(PREFIX)/share/gpe/pixmaps/default/gallery/slideshow.png
	install -m 644 pixmaps/fullscreen.png $(DESTDIR)$(PREFIX)/share/gpe/pixmaps/default/gallery/fullscreen.png
	install -m 644 pixmaps/zoom_in.png $(DESTDIR)$(PREFIX)/share/gpe/pixmaps/default/gallery/zoom_in.png
	install -m 644 pixmaps/zoom_out.png $(DESTDIR)$(PREFIX)/share/gpe/pixmaps/default/gallery/zoom_out.png
	install -m 644 pixmaps/zoom_1.png $(DESTDIR)$(PREFIX)/share/gpe/pixmaps/default/gallery/zoom_1.png
	install -m 644 pixmaps/zoom_fit.png $(DESTDIR)$(PREFIX)/share/gpe/pixmaps/default/gallery/zoom_fit.png
	install -m 644 pixmaps/sharpen.png $(DESTDIR)$(PREFIX)/share/gpe/pixmaps/default/gallery/sharpen.png
	install -m 644 pixmaps/blur.png $(DESTDIR)$(PREFIX)/share/gpe/pixmaps/default/gallery/blur.png
	install -m 644 pixmaps/pan.png $(DESTDIR)$(PREFIX)/share/gpe/pixmaps/default/gallery/pan.png
	install -m 644 pixmaps/rotate.png $(DESTDIR)$(PREFIX)/share/gpe/pixmaps/default/gallery/rotate.png
	strip $(DESTDIR)$(PREFIX)/bin/gpe-gallery

clean:
	rm -f gpe-gallery $(OBJS)

include ../build/Makefile.dpkg_ipkg
include ../build/Makefile.translation
