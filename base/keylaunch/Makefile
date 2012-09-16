PACKAGE   = keylaunch
VERSION   = 2.0.7

#DEBUG     = 1
CVSBUILD  = yes

CC        = gcc
PACKAGE_CFLAGS    = -Os -fomit-frame-pointer -Wall
XROOT     = /usr/X11R6
PACKAGE_CFLAGS  += $(STANDARD_CFLAGS) `pkg-config --cflags xtst xau`
LIBS      = `pkg-config --libs xtst xau x11`

PROG      = keylaunch

PREFIX   ?= /usr/local
INSTALL   = install
RCFILE1	  = .$(PROG)rc
RCFILE2   = /etc/$(PROG)rc
PACKAGE_CFLAGS  += -DRCFILE1=\"$(RCFILE1)\" -DRCFILE2=\"$(RCFILE2)\" -DDEBUG

OBJS      = $(PROG).o xsi.o

ifeq ($(CVSBUILD),yes)
BUILD = ../build
else
BUILD = build
endif

all: $(PROG) 

$(PROG): $(OBJS)
	$(CC) $(OBJS) $(LIBS) -o $@

install-program: all
	$(INSTALL) -D $(PROG) $(DESTDIR)$(PREFIX)/bin/$(PROG)
	$(INSTALL) -D -m 644 $(PROG)rc $(DESTDIR)/etc/$(PROG)rc
	$(INSTALL) -d $(DESTDIR)/etc/X11/Xsession.d
	$(INSTALL) 90$(PROG)  $(DESTDIR)/etc/X11/Xsession.d/90$(PROG)
	$(INSTALL) -d $(DESTDIR)/etc/X11/Xinit.d
	$(INSTALL) 90norepeat  $(DESTDIR)/etc/X11/Xinit.d/90norepeat

clean:
	rm -f $(PROG) $(OBJS)

include $(BUILD)/Makefile.translation
include $(BUILD)/Makefile.dpkg_ipkg
