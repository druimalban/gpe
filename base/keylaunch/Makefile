PACKAGE   = keylaunch
VERSION   = 1.3.0-5gpe9

CC        = gcc
#CFLAGS    = -g -O2 -Wall
CFLAGS    = -Os -fomit-frame-pointer -Wall
XROOT     = /usr/X11R6
INCLUDES  = -I$(XROOT)/include
LIBS      = -lX11 -lXtst
LDPATH    = -L$(XROOT)/lib

PROG      = keylaunch

DESTDIR	  =
PREFIX    = /usr
RCFILE1	  = .$(PROG)rc
RCFILE2   = /etc/$(PROG)rc
DEFINES   = -DRCFILE1=\"$(RCFILE1)\" -DRCFILE2=\"$(RCFILE2)\" -DDEBUG

OBJS      = $(PROG).o xsi.o

all: $(PROG) 

$(PROG): $(OBJS)
	$(CC) $(OBJS) $(LDPATH) $(LIBS) -o $@

$(OBJS): %.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -c $< -o $@

install: all
	install -D $(PROG) $(DESTDIR)$(PREFIX)/bin/$(PROG)
	install -D $(PROG)rc $(DESTDIR)/etc/$(PROG)rc
        install -d $(DESTDIR)/etc/X11/Xsession.d
	install 90$(PROG)  $(DESTDIR)/etc/X11/Xsession.d/90$(PROG)
        install -d $(DESTDIR)/etc/X11/Xinit.d
	install 90norepeat  $(DESTDIR)/etc/X11/Xinit.d/90norepeat

clean:
	rm -f $(PROG) $(OBJS)

old-ipkg:
	strip $(PROG)
	rm -rf familiar/dist
	mkdir -p familiar/dist/CONTROL
	cp familiar/control familiar/dist/CONTROL
	make DESTDIR=`pwd`/familiar/dist PREFIX=/usr install
	chown -R root.root familiar/dist
	ipkg-build familiar/dist

include ../build/Makefile.dpkg_ipkg
