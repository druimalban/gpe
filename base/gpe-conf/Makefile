CFLAGS = `gtk-config --cflags` `gdk-pixbuf-config --cflags` -Os -Wall -fomit-frame-pointer -I../libgpewidget
CC = gcc
MEMBERS  = main applets 
MEMBERS += unimplemented
MEMBERS += timeanddate
MEMBERS += appmgr_setup ../gpe-appmgr/package ../gpe-appmgr/cfg
MEMBERS += ipaqscreen/main ipaqscreen/brightness  ipaqscreen/rotation ipaqscreen/calibrate ipaqscreen/callbacks ipaqscreen/xset 
MEMBERS += kbd
MEMBERS += network
MEMBERS += theme

OBJS = $(patsubst %,%.o,$(MEMBERS))

all: gpe-conf

gpe-conf: $(OBJS)
	$(CC) -o $@ $(OBJS) -L../libgpewidget  -lgpewidget `gdk-pixbuf-config --libs`
	strip gpe-conf

install: gpe-conf
	install -D gpe-conf $(DESTDIR)/usr/bin/gpe-conf

ipkg:
	cp $(HOME)/gpe-conf ./
	cp control dist/CONTROL/control
	make DESTDIR=`pwd`/dist install
	sudo chown -R root.root dist
	ipkg-build dist
	sudo chown -R pierre.pierre dist
clean:
	rm -f gpe-conf $(OBJS) *~ */*~ *.ipk
# to build arm bin on ipaq cluster
tgz:	clean
	cd ..;tar czf /tmp/ss.tgz gpe-conf
