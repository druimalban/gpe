PACKAGE   = keylaunch
VERSION   = 2.0.3

#DEBUG     = 1
CVSBUILD  = yes

CC        = gcc
#CFLAGS    = -g -O2 -Wall
CFLAGS    = -Os -fomit-frame-pointer -Wall
XROOT     = /usr/X11R6
INCLUDES  = -I$(XROOT)/include
LIBS      = -lX11 -lXtst
LDPATH    = -L$(XROOT)/lib

PROG      = keylaunch

DESTDIR	  = /usr/local
PREFIX    = /usr
RCFILE1	  = .$(PROG)rc
RCFILE2   = /etc/$(PROG)rc
DEFINES   = -DRCFILE1=\"$(RCFILE1)\" -DRCFILE2=\"$(RCFILE2)\" -DDEBUG

OBJS      = $(PROG).o xsi.o

ifeq ($(CVSBUILD),yes)
BUILD = ../build
else
BUILD = build
endif

all: $(PROG) 

$(PROG): $(OBJS)
	$(CC) $(OBJS) $(LDPATH) $(LIBS) -o $@

$(OBJS): %.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -c $< -o $@

install-program: all
	install -D $(PROG) $(DESTDIR)$(PREFIX)/bin/$(PROG)
	install -D -m 644 $(PROG)rc $(DESTDIR)/etc/$(PROG)rc
	install -d $(DESTDIR)/etc/X11/Xsession.d
	install 90$(PROG)  $(DESTDIR)/etc/X11/Xsession.d/90$(PROG)
	install -d $(DESTDIR)/etc/X11/Xinit.d
	install 90norepeat  $(DESTDIR)/etc/X11/Xinit.d/90norepeat

clean:
	rm -f $(PROG) $(OBJS)

include $(BUILD)/Makefile.translation
include $(BUILD)/Makefile.dpkg_ipkg
