include ../config.makefile

VERSION = $(shell grep 'Version: ' ../familiar/control | cut -d ' ' -f 2)
PACKAGE = $(shell grep 'Package: ' ../familiar/control | cut -d ' ' -f 2)

SOURCES = $(patsubst %,%.c,$(MEMBERS))
DEPS    = $(patsubst %,%.d,$(MEMBERS))
OBJS    = $(patsubst %,%.o,$(MEMBERS))

CC      = gcc
CFLAGS += -Wall #-g
CFLAGS += -Os
CFLAGS += -MD #(deps)

#-- varaibles
CPPFLAGS += -DPACKAGE=\"$(PACKAGE)\"
CPPFLAGS += -DVERSION=\"$(VERSION)\"
CPPFLAGS += -DPREFIX=\"$(PREFIX)\"
CPPFLAGS += -DPACKAGE_LOCALE_DIR=\"$(PREFIX)/share/locale\"

ifdef DESKTOP #environement variable to set if you want extra code included.
CPPFLAGS += -DDESKTOP
endif

#-- base
LIB_GPE_WIDGET = ../../libgpewidget
CFLAGS += -I$(LIB_GPE_WIDGET)
CFLAGS += `gtk-config --cflags`
LIBS   += -L$(LIB_GPE_WIDGET) -lgpewidget #need to be before gtk libs (overwrite things)
LIBS   += `gtk-config --libs`

#-- GDK pixbuf
CFLAGS += `gdk-pixbuf-config --cflags`
LIBS   += `gdk-pixbuf-config --libs`

#-- SQLite
ifdef USE_SQLITE
LIBS += -lsqlite
endif

#-- libpng support
CPPFLAGS += -I/usr/include
LIBS     += -L/usr/lib -lpng

#-- @ handhelds.org
CPPFLAGS += -I../../include
LIBS     += -L../../lib
LIBS     += -lgdk_pixbuf

#.SUFFIXES: .d

default: $(PACKAGE)

ARM_BIN = $(PACKAGE)-$(VERSION)-arm-striped
arm: $(ARM_BIN)

#--package
$(PACKAGE): $(OBJS)
	$(CC) -o $@ $^ -g $(LIBS)

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
	rm -f $(PACKAGE) $(OBJS) $(DEPS) #$(ARM_BIN)

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

