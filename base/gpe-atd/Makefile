#
# Makefile for atd
#

atd: atd.c
	gcc -s -Wall -Wstrict-prototypes -O2 atd.c -o atd
	rm -f atq
	ln -s atd atq

dist: atd
	cp atd dist/usr/sbin/
	rm dist/usr/bin/atq
	ln -s dist/usr/sbin/atd dist/usr/bin/atq
	fakeroot ./build

clean:
	rm -f atd atq
	rm -rf build
