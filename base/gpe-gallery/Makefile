PREFIX = /usr/local
PACKAGE = gpe-gallery
DEBUG = yes
CVSBUILD = yes
VERSION = 0.91

MEMBERS = main image_tools
GTKCFLAGS = `pkg-config --cflags gtk+-2.0`
GTKLDFLAGS = `pkg-config --libs gtk+-2.0 gdk-2.0`

PACKAGE_CFLAGS += $(STANDARD_CFLAGS) $(GTKCFLAGS) $(GPECFLAGS)
PACKAGE_CPPFLAGS += $(STANDARD_CPPFLAGS) -DPACKAGE=\"$(PACKAGE)\" -DPREFIX=\"$(PREFIX)\" -DPACKAGE_LOCALE_DIR=\"$(PREFIX)/share/locale\"
PACKAGE_LDFLAGS += $(STANDARD_LDFLAGS) $(GTKLDFLAGS) $(GPELIBS)

ifeq ($(CVSBUILD),yes)
BUILD = ../build
else
BUILD = build
endif

OBJS = $(patsubst %,%.o,$(MEMBERS))
SOURCES = $(patsubst %,%.c,$(MEMBERS))
DEPS = $(patsubst %,%.d,$(MEMBERS))

all: $(PACKAGE)

$(PACKAGE): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS) $(PACKAGE_LDFLAGS)

install-program: all
	install -Ds $(PACKAGE) $(DESTDIR)$(PREFIX)/bin/$(PACKAGE)
	install -D -m 644 $(PACKAGE).png $(DESTDIR)$(PREFIX)/share/pixmaps/$(PACKAGE).png
	install -D -m 644 $(PACKAGE).applications $(DESTDIR)/$(PREFIX)/share/application-registry/$(PACKAGE).applications
	mkdir -p $(DESTDIR)$(PREFIX)/share/applications
	install -m 644 $(PACKAGE).desktop $(DESTDIR)$(PREFIX)/share/applications/
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
clean:
	rm -f $(PACKAGE) $(OBJS) $(DEPS)

include $(BUILD)/Makefile.translation
include $(BUILD)/Makefile.dpkg_ipkg
-include $(DEPS)
