PACKAGE = ipaq-sleep

PREFIX = /usr/local

VERSION = 0.7-7

CVSBUILD = yes

ifeq ($(CVSBUILD),yes)
BUILD = ../build
else
BUILD = build
endif

all: $(PACKAGE)

$(PACKAGE): $(PACKAGE).c
	$(CC) -Os -fomit-frame-pointer -o $(PACKAGE) $(PACKAGE).c -I/usr/X11R6/include -L/usr/X11R6/lib -lX11 -lXext -lXss -Wall -lapm

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
