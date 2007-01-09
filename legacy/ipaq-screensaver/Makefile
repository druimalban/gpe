CC = gcc
CFLAGS = -Wall -O2

ipaq-screensaver:	ipaq-screensaver.o

ipkg:	ipaq-screensaver
	strip ipaq-screensaver
	cp ipaq-screensaver dist/usr/bin/
	ipkg-build dist/

clean:
	rm -f *.o
	rm -f ipaq-screensaver dist/usr/bin/ipaq-screensaver
	rm -f *.ipk
