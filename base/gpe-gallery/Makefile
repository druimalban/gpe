PREFIX = /usr/local
PACKAGE = gpe-gallery
DEBUG = yes
CVSBUILD = yes
GTK2 = yes
VERSION = 0.8

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
BUILD = ../build
else
CFLAGS += -I/usr/include/gpe
BUILD = build
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

all: $(PACKAGE)

$(PACKAGE): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

install-program: all
	install -Ds $(PACKAGE) $(DESTDIR)$(PREFIX)/bin/$(PACKAGE)
	install -D -m 644 $(PACKAGE).png $(DESTDIR)$(PREFIX)/share/pixmaps/$(PACKAGE).png
	install -D -m 644 $(PACKAGE).menu $(DESTDIR)$(PREFIX)/lib/menu/$(PACKAGE)
	install -D -m 644 $(PACKAGE).mime $(DESTDIR)/etc/mime-handlers/$(PACKAGE).mime
	mkdir -p $(DESTDIR)$(PREFIX)/share/applications
	install -m 644 $(PACKAGE).desktop $(DESTDIR)$(PREFIX)/share/applications/$(PACKAGE)
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
	install -m 644 pixmaps/list_view.png $(DESTDIR)$(PREFIX)/share/gpe/pixmaps/default/gallery/list_view.png
	install -m 644 pixmaps/icon_view.png $(DESTDIR)$(PREFIX)/share/gpe/pixmaps/default/gallery/icon_view.png

clean:
	rm -f $(PACKAGE) $(OBJS)

include $(BUILD)/Makefile.translation
include $(BUILD)/Makefile.dpkg_ipkg
