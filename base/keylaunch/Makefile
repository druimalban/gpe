CC        = gcc
CFLAGS    = -g -O2 -Wall
XROOT     = /usr/X11R6
INCLUDES  = -I$(XROOT)/include
LIBS      = -lX11 
LDPATH    = -L$(XROOT)/lib

PROG      = keylaunch

DESTDIR	  =
PREFIX    = /usr
RCFILE1	  = .$(PROG)rc
RCFILE2   = /etc/$(PROG)rc
DEFINES   = -DRCFILE1=\"$(RCFILE1)\" -DRCFILE2=\"$(RCFILE2)\" #-DDEBUG

OBJS      = $(PROG).o  

all: $(PROG) 

$(PROG): $(OBJS)
	$(CC) $(OBJS) $(LDPATH) $(LIBS) -o $@

$(OBJS): %.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -c $< -o $@

install: all
	install -D $(PROG) $(DESTDIR)$(PREFIX)/bin/$(PROG)
	install -D $(PROG)rc $(DESTDIR)/etc/$(PROG)rc

clean:
	rm -f $(PROG) $(OBJS)

ipkg:
	strip $(PROG)
	rm -rf familiar/dist
	mkdir -p familiar/dist/CONTROL
	cp familiar/control familiar/dist/CONTROL
	make DESTDIR=`pwd`/familiar/dist PREFIX=/usr install
	chown -R root.root familiar/dist
	ipkg-build familiar/dist
