CFLAGS = `gtk-config --cflags` -Os -Wall -fomit-frame-pointer -I../libgpewidget

MEMBERS = main

OBJS = $(patsubst %,%.o,$(MEMBERS))

all: gpe-mail

gpe-mail: $(OBJS)
	$(CC) -o $@ $< `gtk-config --libs` -lcrypt -lgdk_imlib -L../libgpewidget -lgpewidget

install:
	install -D gpe-mail $(DESTDIR)/usr/bin/gpe-mail
	strip $(DESTDIR)/usr/bin/gpe-mail

ipkg:
	rm -rf dist
	mkdir -p dist/CONTROL
	cp control dist/CONTROL/control
	make DESTDIR=`pwd`/dist install
	chown -R root.root dist
	ipkg-build dist

clean:
	rm -f gpe-mail gpe-mail.o
