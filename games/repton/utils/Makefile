CC=gcc
CFLAGS=-I. -I.. -Wall
LDFLAGS=-s

all: repcopy showsprites

repcopy: repcopy.o ../err.o
	$(CC) $(LDFLAGS) -o repcopy repcopy.o ../err.o

showsprites: showsprites.o ../err.o
	$(CC) $(LDFLAGS) -o showsprites showsprites.o ../err.o -lvga -lvgagl

clean:
	rm -f *.o repcopy showsprites
