#CFLAGS = -I/usr/include/gtk-1.2 -I/usr/include/glib-1.2 -I/usr/lib/glib/include -g
#LDFLAGS= -lgtk -lgdk
LDFLAGS= `gtk-config --libs`
CFLAGS = `gtk-config --cflags` -g

#test: gtkcolombotest

all: clean gpe-mail

l2v.o: l2v.c
	gcc -c -g -o l2v.o l2v.c -DLIB

gpe-mail.o: gpe-mail.c
	gcc $(CFLAGS) -c gpe-mail.c

gpe-mail: gpe-mail.o l2v.o gtkcolombo.o
	gcc -g -o gpe-mail $(LDFLAGS) gpe-mail.o l2v.o gtkcolombo.o

clean:
	/bin/rm -f *.o gpe-mail

gtkcolombo.o: gtkcolombo.c gtkcolombo.h
	gcc $(CFLAGS) -c gtkcolombo.c

gtkcolombotest: gtkcolombo.c gtkcolombo.h
	gcc -DTEST -I/usr/include/gtk-1.2 -I/usr/include/glib-1.2 -I/usr/lib/glib/include -g gtkcolombo.c -o gtkcolombotest -lgtk -lgdk
