include ../config.makefile

CONTROL_FILE = ../familiar/control1

SOURCES = $(patsubst %,%.c,$(MEMBERS))
DEPS    = $(patsubst %,%.d,$(MEMBERS))
OBJS    = $(patsubst %,%.o,$(MEMBERS))

CC      = gcc
CFLAGS += -Wall
CFLAGS += -MD #(deps)

ifeq ($(DEBUG),yes)
CPPFLAGS += -DDEBUG=yes
CFLAGS   += -O2 -g
LDFLAGS  += -g
else
CFLAGS += -Os -fomit-frame-pointer
endif

#-- variables
CPPFLAGS += -DPACKAGE=\"$(PACKAGE)\"
CPPFLAGS += -DVERSION=\"$(VERSION)\"
CPPFLAGS += -DPREFIX=\"$(PREFIX)\"
CPPFLAGS += -DPACKAGE_LOCALE_DIR=\"$(PREFIX)/share/locale\"

ifdef DESKTOP #environement variable to set if you want extra code included.
CPPFLAGS += -DDESKTOP
endif

#-- GPE widgets
ifndef CVSBUILD
CVBUILD = no
endif

ifeq ($(CVSBUILD),yes)
CFLAGS  += -I../../libgpewidget
LDFLAGS += -L../../libgpewidget
endif


LDFLAGS += -lgpewidget #need to be before gtk libs (overwrite things)

#-- GTK2
CFLAGS  +=  #-DGTK_ENABLE_BROKEN
CFLAGS  += `pkg-config --cflags gtk+-2.0 gdk-pixbuf-2.0`
LDFLAGS += `pkg-config --libs   gtk+-2.0 gdk-pixbuf-2.0`

#-- XSettings
ifeq ($(CVSBUILD),yes)
CFLAGS  += -I../../libxsettings -I../../libxsettings-client
LDFLAGS += -L../../libxsettings -L../../libxsettings-client -lXsettings-client
LDFLAGS += -Wl,-rpath-link,../../libxsettings
else
LDFLAGS += -lXsettings-client
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
	$(CC) -o $@ $^ $(LDFLAGS)

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
rc: realclean
realclean: clean
	rm -f $(PACKAGE) #$(ARM_BIN)

-include $(DEPS)

#--reminders
fix: fixme
fixme:
	@echo "NO! that does NOT fix anything! ;)"
	echo "" && grep 'FIXME:' *.h *.c -n
	@echo "+----------------+"
	@echo ' Left to fix: ' `grep 'FIXME:' *.h *.c|wc -l`
	@echo "+----------------+"
note:
	grep 'NOTE:' *.h *.c -n

