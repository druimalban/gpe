include ../config.makefile

CONTROL_FILE = ../familiar/control1

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

CFLAGS  += -I../../libgpewidget
LDFLAGS += -L../../libgpewidget

LDFLAGS += -lgpewidget #need to be before gtk libs (overwrite things)

#-- GTK
CFLAGS  +=  #-DGTK_ENABLE_BROKEN
CFLAGS  += `pkg-config --cflags gtk+-2.0 gdk-pixbuf-2.0`
LDFLAGS += `pkg-config --libs   gtk+-2.0 gdk-pixbuf-2.0`

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
	../utils/dummy-arm-bin &> /dev/null && strip -o $(ARM_BIN) $(PACKAGE)

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

