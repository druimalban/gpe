CC = gcc
CFLAGS = -Wall -O2 -I../include `glib-config --cflags`

OBJS = gpe-exec.o xsi.o gpe-soundgen.o gpe-rtcd.o
#LDFLAGS = `glib-config --libs` -L/usr/X11R6/lib -lX11

libgpe.so: $(OBJS)
	$(CC) -shared -Wl,-soname -Wl,libgpe.so -o libgpe.so $(OBJS) $(LDFLAGS)

clean:
	rm -f *.o *.so
