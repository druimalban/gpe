PREFIX = /usr/local
PACKAGE = xserver-common
CVSBUILD = yes
VERSION = 1.15
DEBUG = no

LINGUAS = 

ifeq ($(CVSBUILD),yes)
BUILD = ../build
else
BUILD = build
endif

all:

install-program: 
	for i in X11/Xsession.d X11/Xinit.d; do install -d $(DESTDIR)/etc/$$i; FILES=`echo $$i/* | sed "s:$$i/CVS::"`; install -m 755 $$FILES $(DESTDIR)/etc/$$i/; done
	install -d $(DESTDIR)$(PREFIX)/bin
	install -m 755 run-calibrate.sh $(DESTDIR)$(PREFIX)/bin/run-calibrate.sh
	install -m 644 X11/Xdefaults $(DESTDIR)/etc/X11/Xdefaults
	install -m 755 X11/Xinit $(DESTDIR)/etc/X11/Xinit
	install -m 755 X11/Xserver $(DESTDIR)/etc/X11/Xserver
	install -m 755 X11/Xsession $(DESTDIR)/etc/X11/Xsession
	install -m 644 X11/*.xmodmap $(DESTDIR)/etc/X11/
	install -m 644 X11/xmodmap-* $(DESTDIR)/etc/X11/

clean:

include $(BUILD)/Makefile.dpkg_ipkg
