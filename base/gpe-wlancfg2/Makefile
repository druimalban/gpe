# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Library General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

PACKAGE = gpe-wlancfg
PREFIX = /usr/local
DEBUG = no
LINGUAS = cs
VERSION = 0.2.3
GTK2 = yes
BUILD = ../build

YACC = byacc
LEX  = flex
CC   = gcc
STRIP = strip

CONTROL = control1

#GTKCFLAGS = `pkg-config --cflags gtk+-2.0` `pkg-config --cflags gthread-2.0` -DGTK_ENABLE_BROKEN
#GTKLDFLAGS += `pkg-config --libs gtk+-2.0` `pkg-config --libs gthread-2.0`
GTKCFLAGS = `pkg-config --cflags gtk+-2.0` 
GTKLDFLAGS += `pkg-config --libs gtk+-2.0` -lfreetype 
#-lXinerama

CPPFLAGS = $(GTKCFLAGS) -D_GNU_SOURCE -I../libgpewidget 
ifeq ($(DEBUG),yes)
CFLAGS += -O2 -g
else
CFLAGS += -Os -fomit-frame-pointer 
LDFLAGS = -L/skiff/local/arm-linux/lib/X11 -lgdk-x11-2.0 -lX11 -lXrender -lXext -lXinerama
endif

CFLAGS += -Wall
CFLAGS += -DPACKAGE=\"$(PACKAGE)\" -DPREFIX=\"$(PREFIX)\" -DPACKAGE_LOCALE_DIR=\"$(PREFIX)/share/locale\" -DPACKAGE_DATA_DIR=\"$(PREFIX)/share/\" -DENABLE_NLS
CFLAGS += -MD

LEXFLAGS = -L
YACCFLAGS = -ld
LEX_OUTPUT_ROOT = lex.yy

.SUFFIXES: .d .c .l .y

MEMBERS = main interface support helptext config-parser callbacks confpars_wl confscan_wl
OBJS = $(patsubst %,%.o,$(MEMBERS))
DEPS = $(patsubst %,%.d,$(MEMBERS))
SOURCES = $(patsubst %,%.c,$(MEMBERS))

PIXMAPS = 

all: $(PACKAGE)

$(PACKAGE): $(OBJS)
	$(CC) -o $@ $^ -L../libgpewidget -lgpewidget -lfl $(GTKLDFLAGS) $(LDFLAGS) 

.l.c:
	$(LEX) $(LEXFLAGS) $< && mv $(LEX_OUTPUT_ROOT).c $@
.y.c:
	$(YACC) $(YACCFLAGS) $< && mv y.tab.c $*.c
	if test -f y.tab.h; then \
	if cmp -s y.tab.h $*.h; then rm -f y.tab.h; else mv y.tab.h $*.h; fi; \
	else :; fi

install-program: all 
	install -D $(PACKAGE) $(DESTDIR)$(PREFIX)/bin/$(PACKAGE)
	$(STRIP) $(DESTDIR)$(PREFIX)/bin/$(PACKAGE)
	mkdir -p $(DESTDIR)$(PREFIX)/share/pixmaps
	install -m 644 $(PACKAGE).png $(DESTDIR)$(PREFIX)/share/pixmaps/$(PACKAGE).png
	mkdir -p $(DESTDIR)$(PREFIX)/share/applications
	install -m 644 $(PACKAGE).desktop $(DESTDIR)$(PREFIX)/share/applications/


clean:
	rm -f $(PACKAGE) $(OBJS) $(DEPS)

ipkg-old: clean
	rm -rf familiar/dist
	mkdir -p familiar/dist/CONTROL
	sed -e s/VERSION/$(VERSION)/ < familiar/$(CONTROL) > familiar/dist/CONTROL/control
	if test -e familiar/conffiles; then install -m 644 familiar/conffiles familiar/dist/CONTROL; fi
	if test -e familiar/preinst;   then install familiar/preinst   familiar/dist/CONTROL; fi
	if test -e familiar/postinst;  then install familiar/postinst  familiar/dist/CONTROL; fi
	if test -e familiar/prerm;     then install familiar/prerm     familiar/dist/CONTROL; fi
	if test -e familiar/postrm;    then install familiar/postrm    familiar/dist/CONTROL; fi
	make DESTDIR=`pwd`/familiar/dist PREFIX=/usr prefix=/usr DEBUG=no install
	$(STRIP) familiar/dist/usr/bin/gpe-wlancfg
	chown -R root.root familiar/dist
	ipkg-build familiar/dist


include $(BUILD)/Makefile.dpkg_ipkg
include $(BUILD)/Makefile.translation

-include $(DEPS)
