VERSION = 0.0.2

PREFIX = "/usr/local"
CFLAGS = `gtk-config --cflags` `gdk-pixbuf-config --cflags` -Os -Wall -fomit-frame-pointer -I../libgpewidget -DVERSION=\"$(VERSION)\" -DPREFIX=\"$(PREFIX)\"
CC = gcc
MEMBERS  = main applets parser
MEMBERS += unimplemented
MEMBERS += timeanddate
MEMBERS += appmgr_setup ../gpe-appmgr/package ../gpe-appmgr/cfg
MEMBERS += ipaqscreen/main ipaqscreen/brightness  ipaqscreen/rotation ipaqscreen/calibrate ipaqscreen/callbacks ipaqscreen/xset 
MEMBERS += kbd
MEMBERS += network
MEMBERS += theme
MEMBERS += keyctl
MEMBERS += ownerinfo
MEMBERS += sleep/main sleep/interface sleep/callbacks sleep/conf sleep/confGUI 
MEMBERS += users/interface users/callbacks  

OBJS = $(patsubst %,%.o,$(MEMBERS))

all: gpe-conf gpe-confsuid

gpe-conf: $(OBJS)
	$(CC) -o $@ $(OBJS) -L../libgpewidget  -lgpewidget `gdk-pixbuf-config --libs` -lcrypt
	strip gpe-conf

gpe-confsuid: gpe-confsuid.c
	$(CC) -o $@ gpe-confsuid.c 
	strip gpe-confsuid

install: gpe-conf
	install -D gpe-conf $(DESTDIR)/usr/bin/gpe-conf
	sudo chown root.root gpe-confsuid
	sudo chmod a+s gpe-confsuid 
	install -D gpe-conf $(DESTDIR)/usr/bin/gpe-confsuid

ipkgl:
	cp $(HOME)/gpe-conf ./
	cp controll distl/CONTROL/control
	make DESTDIR=`pwd`/distl install
	sudo chown -R root.root distl
	ipkg-build distl
	sudo chown -R pierre.pierre distl
ipkg:
	./gpe-conf jhdgkjhd | ./buildmenu.pl >dist/usr/lib/menu/gpe-conf
	find dist -name CVS |xargs rm -rf
	cp $(HOME)/gpe-conf ./
	sed -e s/VERSION/$(VERSION)/ <control >dist/CONTROL/control
	make DESTDIR=`pwd`/dist install
	sudo chown -R root.root dist
	ipkg-build dist
	sudo chown -R pierre.pierre dist
clean:
	rm -f gpe-conf $(OBJS) *~ */*~ *.ipk
# to build arm bin on ipaq cluster
tgz:	clean
	cd ..;tar czf /tmp/ss.tgz gpe-conf
