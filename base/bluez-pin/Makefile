all: bluez-pin

bluez-pin: bluez-pin.o
	$(CC) -o $@ $< `gtk-config --libs` -lsqlite

bluez-pin.o: bluez-pin.c
	$(CC) -c -o $@ $< `gtk-config --cflags` -Os -Wall -fomit-frame-pointer

clean:
	rm bluez-pin bluez-pin.o

