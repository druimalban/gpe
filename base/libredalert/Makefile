CFLAGS=-g `glib-config --cflags`
LDFLAGS=`glib-config --libs`
all: test

test: test.o redalert.o
	gcc -o test $(LDFLAGS) test.o redalert.o

clean:
	rm -f test *.o