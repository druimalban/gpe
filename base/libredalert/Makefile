CFLAGS=-g `pkg-config --cflags glib-2.0`
LDFLAGS=`pkg-config --libs glib-2.0`
all: test

test: test.o redalert.o
	gcc -o test $(LDFLAGS) test.o redalert.o

clean:
	rm -f test *.o
