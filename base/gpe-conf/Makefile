PACKAGE = gpe-conf
VERSION = 0.1.0-pre2
NATIVE = yes
CVSBUILD = yes
DEBUG = no
GTK2 = yes
IPAQ_INCLUDES = -I/home/fuchs/ipaq/linux/include
INSTALL = install
PREFIX = /usr/
DESTDIR = /

LINGUAS = pt ro

ifeq ($(CVSBUILD),yes)
LDFLAGS += -L../libgpewidget -L../libxsettings  -L../libxsettings-client -L../libdotdesktop -L../libdm -ldm -L.
CFLAGS += -Wall -I../libgpewidget -I../libxsettings-client -I../libxsettings -I../libdotdesktop
BUILD = ../build
else
CFLAGS += -I/usr/local/include/gpe
BUILD = build
endif

ifeq ($(DEBUG),yes)
CFLAGS += -g -DDEBUG
else
CFLAGS += -Os -fomit-frame-pointer
endif

LDFLAGS += -lXsettings -lXsettings-client -lcrypt
CFLAGS += -DVERSION=\"$(VERSION)\" -DPREFIX=\"$(PREFIX)\" -D_GNU_SOURCE 

ifeq ($(GTK2),yes)
GTKCFLAGS = `pkg-config --cflags gtk+-2.0` 
GTKLDFLAGS += `pkg-config --libs gtk+-2.0 gdk-2.0` 
CONTROL = control1
else
GTKCFLAGS = `gtk-config --cflags` `gdk-pixbuf-config --cflags`
GTKLDFLAGS = `gtk-config --libs` 
CONTROL = control
endif

# setup how to compile
ifeq ($(NATIVE),yes)

STRIP=strip
CC=gcc

CFLAGS += `pkg-config --cflags gtk+-2.0 gdk-pixbuf-2.0` -Wall
LDFLAGS += -lgpewidget `pkg-config --libs gtk+-2.0 gdk-pixbuf-2.0` -ldotdesktop 

else # we do cross-compile...

CC=arm-linux-gcc
STRIP=arm-linux-strip

CFLAGS += $(IPAQ_INCLUDES)
CFLAGS += `pkg-config --cflags gtk+-2.0 gdk-pixbuf-2.0` -Wall 
LDFLAGS += -L/skiff/local/arm-linux/lib -L/skiff/local/arm-linux/lib/X11  
LDFLAGS += -lgpewidget -lgtk-x11-2.0 -lgdk-x11-2.0 -latk-1.0 -lpangoxft-1.0 
LDFLAGS += -lpangox-1.0 -lpango-1.0 -lgdk_pixbuf-2.0 -lm -lgobject-2.0 
LDFLAGS += -lgmodule-2.0 -ldl -lglib-2.0 -ldotdesktop -lXinerama
LDFLAGS += -lX11 -lXext -lXrender -lgdk-x11-2.0

endif #native or cross-compile

CPPFLAGS += -I.

MEMBERS  = main applets parser suid
MEMBERS += misc
MEMBERS += unimplemented
MEMBERS += timeanddate
MEMBERS += appmgr_setup ../gpe-appmgr/package 
MEMBERS += ipaqscreen/main ipaqscreen/brightness  ipaqscreen/rotation ipaqscreen/calibrate ipaqscreen/callbacks ipaqscreen/xset 
MEMBERS += kbd keyboard
MEMBERS += network
MEMBERS += theme
MEMBERS += keyctl
MEMBERS += ownerinfo
MEMBERS += login-setup
MEMBERS += sleep/main sleep/interface sleep/callbacks sleep/conf sleep/confGUI 
MEMBERS += users/interface users/callbacks  
MEMBERS += cfgfile
MEMBERS += gpe-admin
MEMBERS += storage
MEMBERS += widgets/sp-color-slider
MEMBERS += serial
MEMBERS += logread
MEMBERS += packages cardinfo
MEMBERS += tools/interface

OBJS = $(patsubst %,%.o,$(MEMBERS))
DEPS = $(patsubst %,%.d,$(MEMBERS))
SOURCES = $(patsubst %,%.c,$(MEMBERS))


PIXMAPS = gpe-config-apm.png
PIXMAPS += gpe-config-appmgr.png
PIXMAPS += gpe-config-ipaqscreen.png
PIXMAPS += gpe-config-kbd.png
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
PIXMAPS += gpe-config-users.png
PIXMAPS += gpe-config-login-setup.png
PIXMAPS += gpe-config-admin.png
PIXMAPS += gpe-config-storage.png
PIXMAPS += gpe-config-serial.png
PIXMAPS += gpe-config-packages.png
PIXMAPS += gpe-config-cardinfo.png
PIXMAPS += menu-assign.png menu-eject.png menu-insert.png 
PIXMAPS += menu-reset2.png menu-reset.png menu-resume.png menu-suspend.png
PIXMAPS += pccard-cdrom.png pccard-ide.png pccard-memory.png
PIXMAPS += pccard-parport.png pccard-scsi.png pccard-unknown.png
PIXMAPS += pccard-network.png
PIXMAPS += gpe-config-logread.png
PIXMAPS += gpe-config-keyboard.png

all: $(PACKAGE) 

gpe-conf: $(OBJS)
	$(CC) -g -o $@ $(OBJS) $(LDFLAGS)
	cp gpe-conf gpe-conf.pc
	$(STRIP) gpe-conf

gpe-confsuid: gpe-confsuid.c
	$(CC) -o $@ gpe-confsuid.c 
	$(STRIP) gpe-confsuid

install-program: gpe-conf
	sed -e s/VERSION/$(VERSION)/ <$(CONTROL).pre > $(CONTROL)
	mkdir -p $(DESTDIR)/etc/gpe
	$(INSTALL) -m 644 familiar/gpe-config.conf $(DESTDIR)/etc/gpe/
	$(INSTALL) -m 644 keyboards.def $(DESTDIR)/etc/gpe/
	mkdir -p $(DESTDIR)/$(PREFIX)/bin
	$(INSTALL) gpe-conf $(DESTDIR)$(PREFIX)/bin/gpe-conf
	$(STRIP) $(DESTDIR)$(PREFIX)/bin/gpe-conf
	mkdir -p $(DESTDIR)$(PREFIX)/lib/menu
	mkdir -p $(DESTDIR)$(PREFIX)/share/pixmaps
	for i in $(PIXMAPS); do install -m 644 -D pixmaps/$$i $(DESTDIR)$(PREFIX)/share/pixmaps/$$i; done
	export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:../libgpewidget/
	$(INSTALL) -m 644 gpe-conf.menu_tmp $(DESTDIR)$(PREFIX)/lib/menu/gpe-conf
	mkdir -p  $(DESTDIR)$(PREFIX)/share/gpe/pixmaps/default
	$(INSTALL) -m 644 pixmaps/ipaq.png $(DESTDIR)$(PREFIX)/share/gpe/pixmaps/default/ipaq.png
	mkdir -p $(DESTDIR)$(PREFIX)/share/applications
	$(INSTALL) -m 644 *.desktop $(DESTDIR)$(PREFIX)/share/applications

clean:
	rm -f gpe-conf $(OBJS) *~ */*~ *.ipk *.o

include $(BUILD)/Makefile.dpkg_ipkg
include $(BUILD)/Makefile.translation
