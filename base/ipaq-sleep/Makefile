PACKAGE = ipaq-sleep

PREFIX = /usr/local

VERSION = 0.8

CVSBUILD = yes

PACKAGE_LDFLAGS = $(STANDARD_LIBS) `pkg-config --libs x11 xextensions` -lX11 -lXext -Wall -lapm -lXss
PACKAGE_CFLAGS = $(STANDARD_CFLAGS) `pkg-config --cflags x11 xextensions`
PACKAGE_CPPFLAGS = $(STANDARD_CPPFLAGS)

ifeq ($(CVSBUILD),yes)
BUILD = ../build
else
BUILD = build
endif

OBJS = $(PACKAGE).o

all: $(PACKAGE)

$(PACKAGE): $(OBJS)
	$(CC) -o $(PACKAGE) $(PACKAGE).o $(LDFLAGS) $(PACKAGE_LDFLAGS)

install-program: all
	install -Ds $(PACKAGE) $(DESTDIR)$(PREFIX)/bin/$(PACKAGE)
	mkdir -p $(DESTDIR)/etc/init.d
	install $(PACKAGE).init $(DESTDIR)/etc/init.d/$(PACKAGE)
	install $(PACKAGE).conf $(DESTDIR)/etc/$(PACKAGE).conf
	install -D $(PACKAGE).session $(DESTDIR)/etc/X11/Xsession.d/69ipaq-sleep

clean:
	rm -f $(PACKAGE) $(OBJS)

include $(BUILD)/Makefile.translation
include $(BUILD)/Makefile.dpkg_ipkg
