CC = gcc
CFLAGS = -Wall -O2 -I../include

OBJS = gpe-test.o
LDFLAGS = -L../src -lgpe `glib-config --libs` -L/usr/X11R6/lib -lX11

all: testcmd gpe-test

testcmd: testcmd.o

gpe-test: $(OBJS)
	$(CC) -o gpe-test $(OBJS) $(LDFLAGS)

clean:
	rm -f *.o gpe-test