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

PACKAGE = gpe-db
VERSION = 2.0
PREFIX = /usr/local
DEBUG = no
CVSBUILD = yes

LINGUAS = de

MEMBERS = main interface callbacks support db-funcs

GTKCFLAGS = `pkg-config --cflags gtk+-2.0` `pkg-config --cflags gthread-2.0` -DGTK_ENABLE_BROKEN
GTKLDFLAGS += -lX11 `pkg-config --libs gtk+-2.0` `pkg-config --libs gthread-2.0`

CPPFLAGS = $(GTKCFLAGS) -D_GNU_SOURCE -DENABLE_NLS -I../libgpewidget
ifeq ($(DEBUG),yes)
CFLAGS += -O2 -g
LDFLAGS = -g -L/skiff/local/arm-linux/lib/X11 #-lefence
else
CFLAGS += -Os -fomit-frame-pointer
LDFLAGS = -lXrender -lXext -lXinerama
endif
CFLAGS += -Wall
CPPFLAGS += -DPACKAGE=\"$(PACKAGE)\" -DPREFIX=\"$(PREFIX)\" -DPACKAGE_LOCALE_DIR=\"$(PREFIX)/share/locale\"
CFLAGS += -MD

.SUFFIXES: .d

OBJS = $(patsubst %,%.o,$(MEMBERS))
DEPS = $(patsubst %,%.d,$(MEMBERS))
SOURCES = $(patsubst %,%.c,$(MEMBERS))

ifeq ($(CVSBUILD),yes)
BUILD = ../build
else
BUILD = build
endif

#PIXMAPS = bell.png

all: $(PACKAGE)

$(PACKAGE): $(OBJS)
	$(CC) -o $@ $^ -L/usr/X11R6/lib -L../libgpewidget -lgpewidget $(GTKLDFLAGS) -lsqlite $(LDFLAGS)
#	$(CC) -o $@ $^ -L/skiff/local/arm-linux/lib/X11 -L../libgpewidget -lgpewidget $(GTKLDFLAGS) -lsqlite $(LDFLAGS)

install-program: all 
#	for i in $(PIXMAPS); do install -D pixmaps/$$i $(DESTDIR)$(PREFIX)/share/$(PACKAGE)/pixmaps/$$i; done
	install -Ds $(PACKAGE) $(DESTDIR)$(PREFIX)/bin/$(PACKAGE)

clean:
	rm -f $(PACKAGE) $(OBJS) $(DEPS)

include $(BUILD)/Makefile.translation
include $(BUILD)/Makefile.dpkg_ipkg
-include $(DEPS)
