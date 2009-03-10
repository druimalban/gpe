CC = gcc
CFLAGS = -Wall -O2 -I../include `glib-config --cflags` -I../../libgpewidget

USE_USQLD = yes

ifeq ($(USE_USQLD),yes) 
CFLAGS += -DUSE_USQLD -I../../usqld/src
LDFLAGS += -lusqld
else 
LDFLAGS += -lsqlite
endif

OBJS = gpe-sql.o

libgpedb.so.0: $(OBJS)
	$(CC) -nostartfiles -shared -Wl,-soname -Wl,libgpedb.so.0 -o libgpedb.so.0 $(OBJS)
	ln -s libgpedb.so.0 libgpedb.so

install: libgpedb.so.0
	install -D libgpedb.so.0 $(DESTDIR)$(PREFIX)/lib/libgpedb.so.0
	#strip $(DESTDIR)$(PREFIX)/lib/libgpedb.so.0

ipkg:
	rm -rf familiar/dist
	mkdir -p familiar/dist/CONTROL
	cp familiar/control familiar/postinst familiar/dist/CONTROL
	make DESTDIR=`pwd`/familiar/dist PREFIX=/usr install
	chown -R root.root familiar/dist
	ipkg-build familiar/dist

clean:
	rm -rf *.o *.so *.so.0 *.ipk familiar/dist
