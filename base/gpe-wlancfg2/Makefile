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
LINGUAS = de pt_PT

YACC = byacc
LEX  = flex
CC   = gcc

#GTKCFLAGS = `pkg-config --cflags gtk+-2.0` `pkg-config --cflags gthread-2.0` -DGTK_ENABLE_BROKEN
#GTKLDFLAGS += `pkg-config --libs gtk+-2.0` `pkg-config --libs gthread-2.0`
GTKCFLAGS = `pkg-config --cflags gtk+-2.0` 
GTKLDFLAGS += `pkg-config --libs gtk+-2.0` -lfreetype
CONTROL = control

CPPFLAGS = $(GTKCFLAGS) -D_GNU_SOURCE -I../libgpewidget 
ifeq ($(DEBUG),yes)
CFLAGS += -O2 -g
LDFLAGS = -g #-lefence
else
CFLAGS += -Os -fomit-frame-pointer
endif

CFLAGS += -Wall
CPPFLAGS += -DPACKAGE=\"$(PACKAGE)\" -DPREFIX=\"$(PREFIX)\" -DPACKAGE_LOCALE_DIR=\"$(PREFIX)/share/locale\" -DPACKAGE_DATA_DIR=\"$(PREFIX)/share/\"
CFLAGS += -MD

LEXFLAGS = -L
YACCFLAGS = -ld
LEX_OUTPUT_ROOT = lex.yy

.SUFFIXES: .d .c .l .y

MEMBERS = main interface support config-parser callbacks confpars_wl confscan_wl
OBJS = $(patsubst %,%.o,$(MEMBERS))
DEPS = $(patsubst %,%.d,$(MEMBERS))

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


install: all
	for i in $(PIXMAPS); do install -D pixmaps/$$i $(DESTDIR)$(PREFIX)/share/$(PACKAGE)/pixmaps/$$i; done
	install -D $(PACKAGE) $(DESTDIR)$(PREFIX)/bin/$(PACKAGE)
	strip $(DESTDIR)$(PREFIX)/bin/$(PACKAGE)

clean:
	rm -f $(PACKAGE) $(OBJS) $(DEPS)

include ../build/Makefile.dpkg_ipkg
-include $(DEPS)
