#
# Makefile for atd
#

atd: atd.c rtc.h
	gcc -s -Wall -Wstrict-prototypes -O2 atd.c -o atd
	rm atq
	ln -s atd atq

dist: atd
	cp atd dist/usr/sbin/
	rm dist/usr/bin/atq
	ln -s dist/usr/sbin/atd dist/usr/bin/atq
	fakeroot ./build
