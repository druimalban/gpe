VERSION = 0.0.5

ifndef PREFIX # simple test to recognise if we are from ipaq
PREFIX = "/usr/local"
endif

ifeq ($(GTK2),yes)
GTKCFLAGS = `pkg-config --cflags gtk+-2.0`-DGTK_ENABLE_BROKEN
GTKLDFLAGS += `pkg-config --libs gtk+-2.0`
CONTROL = control1
else
GTKCFLAGS = `gtk-config --cflags` `gdk-pixbuf-config --cflags`
GTKLDFLAGS = `gtk-config --libs`
CONTROL = control
endif

CFLAGS = $(GTKCFLAGS) -D_GNU_SOURCE -Wall -fomit-frame-pointer -I../libgpewidget -DVERSION=\"$(VERSION)\" -DPREFIX=\"$(PREFIX)\"
LDFLAGS = $(GTKLDFLAGS) -L../libgpewidget  -lgpewidget -lcrypt
CC = gcc

MEMBERS  = main applets parser suid
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

ARM_BINARY= gpe-conf 

ARM_BINARYPATHED = $(patsubst %,/armbins/%,$(ARM_BINARY))
all: gpe-conf 

gpe-conf: $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)
	strip gpe-conf
	cp gpe-conf gpe-conf.pc
	
gpe-confsuid: gpe-confsuid.c
	$(CC) -o $@ gpe-confsuid.c 
	strip gpe-confsuid

install: gpe-conf
	./gpe-conf.pc jhdgkjhd | ./buildmenu.pl >$(DESTDIR)/usr/lib/menu/gpe-conf #build the menu
	sudo chown root.root gpe-conf
	sudo chmod a+s gpe-conf 
	sudo chmod a+x gpe-conf 
	install -D gpe-conf $(DESTDIR)/usr/bin/gpe-conf

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
send:
	scp $(ARM_BINARY) $(HOMEUSER)@$(pc):/armbins/
