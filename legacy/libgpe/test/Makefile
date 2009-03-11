CC = gcc
CFLAGS = -Wall -O2 -I../include

OBJS = gpe-test.o
LDFLAGS = -L../src -lgpe `glib-config --libs` -L/usr/X11R6/lib -lX11

all: testcmd gpe-test test-prefs

testcmd: testcmd.o

gpe-test: $(OBJS)
	$(CC) -o gpe-test $(OBJS) $(LDFLAGS)

___WHY_DO_I_HAVE_TO_SET_THIS___ =  -L/usr/X11R6/lib -lX11
test-prefs: test-prefs.c
	$(CC) -o test-prefs test-prefs.c $(CFLAGS) `pkg-config --cflags glib-2.0` -L../src -lgpe `pkg-config --libs glib-2.0` $(___WHY_DO_I_HAVE_TO_SET_THIS___)

clean:
	rm -f *.o gpe-test test-prefs
