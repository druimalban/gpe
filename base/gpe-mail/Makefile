#CFLAGS = -I/usr/include/gtk-1.2 -I/usr/include/glib-1.2 -I/usr/lib/glib/include -g
#LDFLAGS= -lgtk -lgdk
#LDFLAGS= `gtk-config --libs`
#CFLAGS = `gtk-config --cflags` -g

CFLAGS2 = `pkg-config --cflags gtk+-2.0` -g -Wall -Werror
LDFLAGS2 = `pkg-config --libs gtk+-2.0`

CFLAGS1 = `gtk-config --cflags` `gdk-pixbuf-config --cflags` -DGTK1
LDFLAGS1 = `gtk-config --libs`

CFLAGS=$(CFLAGS2)
LDFLAGS=$(LDFLAGS2)

#test: gtkcolombotest

all: clean gpe-mail

l2v.o: l2v.c
	gcc -c -g -o l2v.o l2v.c -DLIB

gpe-mail.o: gpe-mail.c
	gcc $(CFLAGS) -c gpe-mail.c

gpe-mail: gpe-mail.o gtkcolombo.o
	gcc -g -o gpe-mail $(LDFLAGS) gpe-mail.o gtkcolombo.o

clean:
	/bin/rm -f *.o gpe-mail

gtkcolombo.o: gtkcolombo.c gtkcolombo.h
	gcc $(CFLAGS) -c gtkcolombo.c

gtkcolombotest: gtkcolombotest1 gtkcolombotest2

gtkcolombotest1: gtkcolombo.c gtkcolombo.h
	gcc -DTEST -DGTK1 $(CFLAGS1) $(LDFLAGS1)  gtkcolombo.c -o gtkcolombotest1

gtkcolombotest2: gtkcolombo.c gtkcolombo.h
	gcc -DTEST $(CFLAGS2) $(LDFLAGS2)  gtkcolombo.c -o gtkcolombotest2
