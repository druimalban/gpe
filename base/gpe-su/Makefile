PREFIX = /usr/local
PACKAGE = gpe-su
DEBUG = yes
CVSBUILD = yes
VERSION = 0.10

#FIXME: add: ru sr
LINGUAS = nl pt fr sk cs de sv ro 

MEMBERS = gpe-su

GTKCFLAGS = `pkg-config --cflags gtk+-2.0 gdk-2.0`
GTKLDFLAGS = `pkg-config --libs gtk+-2.0 gdk-2.0`

PACKAGE_CPPFLAGS = $(STANDARD_CPPFLAGS)
PACKAGE_CFLAGS += $(STANDARD_CFLAGS) $(GTKCFLAGS) $(GPECFLAGS)
PACKAGE_LDFLAGS += $(STANDARD_LDFLAGS) $(GPELIBS) $(GTKLDFLAGS)

PACKAGE_LDFLAGS += -lutil

ifeq ($(CVSBUILD),yes)
BUILD = ../build
else
BUILD = build
endif

DEPS = $(patsubst %,%.d,$(MEMBERS))
OBJS = $(patsubst %,%.o,$(MEMBERS))
SOURCES = $(patsubst %,%.c,$(MEMBERS))

all: $(PACKAGE)

$(PACKAGE): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS) $(PACKAGE_LDFLAGS)

clean:
	rm -f $(PACKAGE) $(OBJS) $(DEPS)

install-program: all $(PACKAGE).desktop
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	install -s $(PACKAGE) $(DESTDIR)$(PREFIX)/bin/$(PACKAGE)
	mkdir -p $(DESTDIR)$(PREFIX)/share/applications
	install -m 644 $(PACKAGE).desktop $(DESTDIR)$(PREFIX)/share/applications
	mkdir -p $(DESTDIR)$(PREFIX)/share/pixmaps
	install -m 644 $(PACKAGE).png $(DESTDIR)$(PREFIX)/share/pixmaps/$(PACKAGE).png

include $(BUILD)/Makefile.translation
include $(BUILD)/Makefile.dpkg_ipkg
