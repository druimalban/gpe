PACKAGE = gpe-conf
VERSION = 0.0.7

PREFIX = /usr/local
BINDIR=$(PREFIX)/bin
DEBUG = no
INSTALL = install
STRIP = strip
GTK2 = yes

ifeq ($(GTK2),yes)
GTKCFLAGS = `pkg-config --cflags gtk+-2.0` -DGTK_ENABLE_BROKEN
GTKLDFLAGS += `pkg-config --libs gtk+-2.0`
CONTROL = control1
else
GTKCFLAGS = `gtk-config --cflags` `gdk-pixbuf-config --cflags`
GTKLDFLAGS = `gtk-config --libs` 
CONTROL = control
endif

CFLAGS = $(GTKCFLAGS) -D_GNU_SOURCE -Wall -fomit-frame-pointer -I../libgpewidget -DVERSION=\"$(VERSION)\" -DPREFIX=\"$(PREFIX)\"
LDFLAGS = $(GTKLDFLAGS) -L../libgpewidget  -lgpewidget -lcrypt -L../libdotdesktop -ldotdesktop
CC = gcc

MEMBERS  = main applets parser suid
MEMBERS += misc
MEMBERS += unimplemented
MEMBERS += timeanddate
MEMBERS += appmgr_setup ../gpe-appmgr/package ../gpe-appmgr/cfg
MEMBERS += ipaqscreen/main ipaqscreen/brightness  ipaqscreen/rotation ipaqscreen/calibrate ipaqscreen/callbacks ipaqscreen/xset 
MEMBERS += kbd
MEMBERS += network
MEMBERS += theme
MEMBERS += keyctl
MEMBERS += ownerinfo
MEMBERS += login-setup
MEMBERS += sleep/main sleep/interface sleep/callbacks sleep/conf sleep/confGUI 
MEMBERS += users/interface users/callbacks  
MEMBERS += cfgfile

OBJS = $(patsubst %,%.o,$(MEMBERS))
DEPS = $(patsubst %,%.d,$(MEMBERS))
SOURCES = $(patsubst %,%.c,$(MEMBERS))


PIXMAPS = gpe-config-apm.png
PIXMAPS += gpe-config-appmgr.png
PIXMAPS += gpe-config-ipaqscreen.png
PIXMAPS += gpe-config-keyboard.png
PIXMAPS += gpe-config-keyctl.png
PIXMAPS += gpe-config-mixer.png
PIXMAPS += gpe-config-mouse.png
PIXMAPS += gpe-config-network.png
PIXMAPS += gpe-config-ownerinfo.png
PIXMAPS += gpe-config-screensaver.png
PIXMAPS += gpe-config-sleep.png
PIXMAPS += gpe-config-software.png
PIXMAPS += gpe-config-theme.png
PIXMAPS += gpe-config-time.png
PIXMAPS += gpe-config.png

all: $(PACKAGE) 

gpe-conf: $(OBJS)
	$(CC) -g -o $@ $(OBJS) $(LDFLAGS)
	cp gpe-conf gpe-conf.pc
	$(STRIP) gpe-conf

gpe-confsuid: gpe-confsuid.c
	$(CC) -o $@ gpe-confsuid.c 
	$(STRIP) gpe-confsuid

install: gpe-conf
	sed -e s/VERSION/$(VERSION)/ <$(CONTROL).pre > $(CONTROL)
	mkdir -p $(DESTDIR)/etc/gpe
	mkdir -p $(DESTDIR)$(BINDIR)
	$(INSTALL) gpe-conf $(DESTDIR)/$(BINDIR)/gpe-conf
	$(STRIP) $(DESTDIR)/$(BINDIR)/gpe-conf
	mkdir -p $(DESTDIR)$(PREFIX)/lib/menu
	mkdir -p $(DESTDIR)$(PREFIX)/share/pixmaps
	for i in $(PIXMAPS); do install -D pixmaps/$$i $(DESTDIR)$(PREFIX)/share/pixmaps/$$i; done
	export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:../libgpewidget/
	./gpe-conf.pc jhdgkjhd | ./buildmenu.pl > gpe-conf.menu_tmp
	$(INSTALL) gpe-conf.menu_tmp $(DESTDIR)$(PREFIX)/lib/menu/gpe-conf
	mkdir -p  $(DESTDIR)$(PREFIX)/share/gpe/pixmaps/default
	$(INSTALL) pixmaps/ipaq.png $(DESTDIR)$(PREFIX)/share/gpe/pixmaps/default/ipaq.png

ipkg: clean
	rm -rf familiar/dist
	make DESTDIR=`pwd`/familiar/dist PREFIX=/usr prefix=/usr install
	mkdir -p familiar/dist/CONTROL
	cp familiar/$(CONTROL) familiar/dist/CONTROL/control
	if test -e familiar/conffiles; then install -m 644 familiar/conffiles familiar/dist/CONTROL; fi
	if test -e familiar/preinst;   then install familiar/preinst   familiar/dist/CONTROL; fi
	if test -e familiar/postinst;  then install familiar/postinst  familiar/dist/CONTROL; fi
	if test -e familiar/prerm;     then install familiar/prerm     familiar/dist/CONTROL; fi
	if test -e familiar/postrm;    then install familiar/postrm    familiar/dist/CONTROL; fi
	chown -R root.root familiar/dist
	ipkg-build familiar/dist

clean:
	rm -f gpe-conf $(OBJS) *~ */*~ *.ipk

