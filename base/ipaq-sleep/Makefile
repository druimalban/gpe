PACKAGE = ipaq-sleep

PREFIX = /usr/local

all: ipaq-sleep

ipaq-sleep: ipaq-sleep.c
	$(CC) -Os -fomit-frame-pointer -o ipaq-sleep ipaq-sleep.c -I/usr/X11R6/include -L/usr/X11R6/lib -lX11 -lXext -lXss -Wall

install: all
	install -D $(PACKAGE) $(DESTDIR)$(PREFIX)/bin/$(PACKAGE)
	strip $(DESTDIR)$(PREFIX)/bin/$(PACKAGE)
	mkdir -p $(DESTDIR)/etc/init.d
	install ipaq-sleep.init $(DESTDIR)/etc/init.d/ipaq-sleep
	install ipaq-sleep.conf $(DESTDIR)/etc/ipaq-sleep.conf
	install -D ipaq-sleep.session $(DESTDIR)/etc/X11/Xsession.d/69ipaq-sleep

clean:
	rm -f $(PACKAGE) $(OBJS)

include ../build/Makefile.dpkg_ipkg
