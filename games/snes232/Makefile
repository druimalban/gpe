all: micro.hex snes232.o

micro.hex: micro.asm
	avra micro.asm

snes232.o: snes232.c
	$(CC) -D__KERNEL__ -DMODULE -O2 -c snes232.c

ipkg: snes232.o
	rm -rf dist
	mkdir -p dist/CONTROL
	cp control dist/CONTROL
	mkdir -p dist/lib/modules/
	cp snes232.o dist/lib/modules/
	ipkg-build dist

