all:libxdr example

CFLAGS=-Os -fPIC

libxdr:xdr-schema.lo xdr-tree.lo xdr-serialize.lo xdr-deserialize.lo
	gcc -shared -o libxdr.so *.lo
	
xdr-tree.lo: xdr.h xdr-tree.c
	gcc $(CFLAGS) -c xdr-tree.c -o xdr-tree.lo

xdr-schema.lo: xdr.h xdr-schema.c
	gcc $(CFLAGS) -c xdr-schema.c -o xdr-schema.lo

xdr-serialize.lo: xdr.h xdr-serialize.c
	gcc $(CFLAGS) -c xdr-serialize.c -o xdr-serialize.lo

xdr-deserialize.lo: xdr.h xdr-deserialize.c
	gcc $(CFLAGS) -c xdr-deserialize.c -o xdr-deserialize.lo


example: libxdr example.c
	gcc -L./ -lxdr example.c -o example

clean:
	rm -f *.o *.lo *.so example *~
