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

PACKAGE = gpe-announce
PREFIX = /usr
CVSBUILD = yes
DEBUG = no
LINGUAS = 
VERSION = 0.14

MEMBERS = main interface callbacks

PACKAGE_CPPFLAGS = $(STANDARD_CPPFLAGS)
SCHEDULE_CFLAGS=$(shell pkg-config --cflags libgpeschedule)
SCHEDULE_LIBS=$(shell pkg-config --libs libgpeschedule)
PACKAGE_CFLAGS = $(STANDARD_CFLAGS) $(SCHEDULE_CFLAGS) $(GPECFLAGS) `pkg-config --cflags gthread-2.0`  `pkg-config --cflags gstreamer-0.10 esound`
PACKAGE_LDFLAGS = $(STANDARD_LDFLAGS) $(SCHEDULE_LIBS) $(GPELIBS) `pkg-config --libs gthread-2.0 esound`

ifeq ($(CVSBUILD),yes)
PACKAGE_CPPFLAGS += -I../libsoundgen -I../libschedule
PACKAGE_LDFLAGS += -L../libsoundgen -L../libschedule
endif

OBJS = $(patsubst %,%.o,$(MEMBERS))
DEPS = $(patsubst %,%.d,$(MEMBERS))
SOURCES = $(patsubst %,%.c,$(MEMBERS))

ifeq ($(CVSBUILD),yes)
BUILD = ../build
else
BUILD = build
endif

all: $(PACKAGE)

$(PACKAGE): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS) $(PACKAGE_LDFLAGS) -lsoundgen $(SCHEDULE_LIBS) -ldl

install-program: all 
	install -d $(DESTDIR)$(PREFIX)/bin
	install $(PACKAGE) $(DESTDIR)$(PREFIX)/bin/$(PACKAGE)

clean:
	rm -f $(PACKAGE) $(OBJS) $(DEPS)

include $(BUILD)/Makefile.translation
include $(BUILD)/Makefile.dpkg_ipkg
-include $(DEPS)
