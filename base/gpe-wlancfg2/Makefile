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
CVSBUILD = yes
VERSION = 0.2.6
LINGUAS = cs

ifeq ($(CVSBUILD),yes)
BUILD = ../build
else
BUILD = build
endif

YACC = byacc
LEX  = flex

GTKCFLAGS = `pkg-config --cflags gtk+-2.0`
GTKLDFLAGS = `pkg-config --libs gtk+-2.0 gdk-2.0`

PACKAGE_CFLAGS += -Os -Wall $(STANDARD_CFLAGS) $(GTKCFLAGS) 
PACKAGE_CFLAGS += -MD $(OPTIONS) -g  
PACKAGE_CFLAGS += -DPACKAGE=\"$(PACKAGE)\" -DPREFIX=\"$(PREFIX)\" 
PACKAGE_CFLAGS += -DPACKAGE_LOCALE_DIR=\"$(PREFIX)/share/locale\" 
PACKAGE_CFLAGS += -DPACKAGE_DATA_DIR=\"$(PREFIX)/share/\"
PACKAGE_CFLAGS += -DENABLE_NLS
PACKAGE_CFLAGS += $(GPECFLAGS)

PACKAGE_LDFLAGS += $(STANDARD_LDFLAGS) $(GTKLDFLAGS) $(GPELIBS) -lfl

CONTROL = control1

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
	$(CC) -o $@ $^ $(LDFLAGS) $(GPELIBS) -lfl -lXrender -lXinerama	

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

include $(BUILD)/Makefile.dpkg_ipkg
include $(BUILD)/Makefile.translation

-include $(DEPS)
