CC = gcc
CFLAGS = -Wall -O2 -I../include `glib-config --cflags`

OBJS = gpe-exec.o xsi.o gpe-soundgen.o gpe-rtcd.o

libgpe.so.0: $(OBJS)
	$(CC) -nostartfiles -shared -Wl,-soname -Wl,libgpe.so.0 -o libgpe.so.0 $(OBJS)
	ln -s libgpe.so.0 libgpe.so

install: libgpe.so.0
	install -D libgpe.so.0 $(DESTDIR)$(PREFIX)/lib/libgpe.so.0
	strip $(DESTDIR)$(PREFIX)/lib/libgpe.so.0

ipkg:
	rm -rf familiar/dist
	mkdir -p familiar/dist/CONTROL
	cp familiar/control familiar/postinst familiar/dist/CONTROL
	make DESTDIR=`pwd`/familiar/dist PREFIX=/usr install
	chown -R root.root familiar/dist
	ipkg-build familiar/dist

clean:
	rm -rf *.o *.so *.so.0 *.ipk familiar/dist
