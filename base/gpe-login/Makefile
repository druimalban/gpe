gpe-login: gpe-login.o
	$(CC) -o $@ $< `gtk-config --libs` -lcrypt

gpe-login.o: gpe-login.c
	$(CC) -c -o $@ $< `gtk-config --cflags` -Os -Wall -fomit-frame-pointer

install:
	cp gpe-login $(DESTDIR)/usr/bin/

clean:
	rm -f gpe-login gpe-login.o
