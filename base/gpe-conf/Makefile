VERSION = 0.0.3

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

ARM_BINARY= gpe-conf gpe-confsuid

ARM_BINARYPATHED = $(patsubst %,/armbins/%,$(ARM_BINARY))
all: gpe-conf gpe-confsuid

gpe-conf: $(OBJS)
	$(CC) -o $@ $(OBJS) -L../libgpewidget  -lgpewidget `gdk-pixbuf-config --libs` -lcrypt
	strip gpe-conf

gpe-confsuid: gpe-confsuid.c
	$(CC) -o $@ gpe-confsuid.c 
	strip gpe-confsuid

install: gpe-conf
	./gpe-conf jhdgkjhd | ./buildmenu.pl >$(DESTDIR)/usr/lib/menu/gpe-conf #build the menu
	install -D gpe-conf $(DESTDIR)/usr/bin/gpe-conf
	sudo chown root.root gpe-confsuid
	sudo chmod a+s gpe-confsuid 
	install -D gpe-conf $(DESTDIR)/usr/bin/gpe-confsuid

ipkg:
	find dist -name CVS |xargs rm -rf				#erase CVS stuff
	cp /armbins/gpe-conf ./ 					#get the arm binary the ipaq sent us.
	sed -e s/VERSION/$(VERSION)/ <control >dist/CONTROL/control	#change the version of the ipkg
	make DESTDIR=`pwd`/dist install
	sudo chown -R root.root dist
	ipkg-build dist
	sudo chown -R pierre.pierre dist
clean:
	rm -f gpe-conf $(OBJS) *~ */*~ *.ipk
# to build arm bin on ipaq cluster
tgz:	clean
	cd ..;tar czf /tmp/ss.tgz gpe-conf
