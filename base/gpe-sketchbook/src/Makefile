include ../config.makefile

ifeq ($(GTK2),yes)
CONTROL_FILE = ../familiar/control1
else
CONTROL_FILE = ../familiar/control
endif

VERSION = $(shell grep 'Version: ' $(CONTROL_FILE) | cut -d ' ' -f 2)
PACKAGE = gpe-sketchbook
#PACKAGE = $(shell grep 'Package: ' $(CONTROL_FILE) | cut -d ' ' -f 2)

SOURCES = $(patsubst %,%.c,$(MEMBERS))
DEPS    = $(patsubst %,%.d,$(MEMBERS))
OBJS    = $(patsubst %,%.o,$(MEMBERS))

CC      = gcc
CFLAGS += -Wall #-g
CFLAGS += -Os
CFLAGS += -MD #(deps)

#-- variables
CPPFLAGS += -DPACKAGE=\"$(PACKAGE)\"
CPPFLAGS += -DVERSION=\"$(VERSION)\"
CPPFLAGS += -DPREFIX=\"$(PREFIX)\"
CPPFLAGS += -DPACKAGE_LOCALE_DIR=\"$(PREFIX)/share/locale\"

ifdef DESKTOP #environement variable to set if you want extra code included.
CPPFLAGS += -DDESKTOP
endif

#-- GPE widgets
LIB_GPE_WIDGET = ../../libgpewidget
CFLAGS  += -I$(LIB_GPE_WIDGET)
ifeq ($(GTK2),yes)
LDFLAGS += `ln -sf ../../libgpewidget/libgpewidget.so.1 libgpewidget.so`
else
LDFLAGS += `ln -sf ../../libgpewidget/libgpewidget.so.0 libgpewidget.so`
endif
LDFLAGS += -L.

LDFLAGS += -lgpewidget #need to be before gtk libs (overwrite things)

#-- GTK
ifeq ($(GTK2),yes)
CFLAGS  +=  -DGTK2 #-DGTK_ENABLE_BROKEN
CFLAGS  += `pkg-config --cflags gtk+-2.0 gdk-pixbuf-2.0`
LDFLAGS += `pkg-config --libs   gtk+-2.0 gdk-pixbuf-2.0`
else
CFLAGS  += `gtk-config --cflags`
LDFLAGS += `gtk-config --libs`
CFLAGS  += `gdk-pixbuf-config --cflags`
LDFLAGS += `gdk-pixbuf-config --libs`
#libpng
CPPFLAGS += -I/usr/include
LDFLAGS  += -L/usr/lib -lpng
#-- @ handhelds.org
CPPFLAGS += -I../../include
LDFLAGS  += -L../../lib
LDFLAGS  += -lgdk_pixbuf
endif

#-- SQLite
ifdef USE_SQLITE
LDFLAGS += -lsqlite
endif

#.SUFFIXES: .d

default: $(PACKAGE)

ARM_BIN = $(PACKAGE)-$(VERSION)-arm-striped
arm: $(ARM_BIN)

#--package
$(PACKAGE): $(OBJS)
	$(CC) -o $@ $^ -g $(LDFLAGS)

$(ARM_BIN):
	../utils/dummy-arm-bin &> /dev/null && make $(PACKAGE)
	../utils/dummy-arm-bin &> /dev/null && cp   $(PACKAGE) $(ARM_BIN)
	../utils/dummy-arm-bin &> /dev/null && strip $(ARM_BIN)

strip: $(PACKAGE)
	strip $(PACKAGE)

x: $(PACKAGE)
	./$(PACKAGE)

clean:
	rm -f *~
	rm -f $(OBJS) $(DEPS)

realclean: clean
	rm -f $(PACKAGE) #$(ARM_BIN)

-include $(DEPS)


#--handhelds.org
trans-pc-to-handheld: dist
	~/bin/move-TO-handhelds.org `pwd` $(DIST_NAME).tar.gz

trans-handheld-to-pc:
	~/bin/move-FROM-handhelds.org $(DIST_NAME) src/$(PACKAGE)-$(VERSION)-arm-striped


#--reminders
fix: fixme
fixme:
	@echo "NO! that does NOT fix anything! ;)"
	grep 'FIXME:' *.h *.c -n
	@echo "+----------------+"
	@echo ' Left to fix: ' `grep 'FIXME:' *.h *.c|wc -l`
	@echo "+----------------+"
note:
	grep 'NOTE:' *.h *.c -n


#-- hooks to main makefile targets
dist:
	cd .. && make dist
ipkg:
	cd .. && make ipkg

