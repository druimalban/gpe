include ../config.makefile

CONTROL_FILE = ../familiar/control1

SOURCES = $(patsubst %,%.c,$(MEMBERS))
DEPS    = $(patsubst %,%.d,$(MEMBERS))
OBJS    = $(patsubst %,%.o,$(MEMBERS))

PACKAGE_CFLAGS += -Wall -D_GNU_SOURCE
PACKAGE_CFLAGS += -MD #(deps)

ifeq ($(DEBUG),yes)
CPPFLAGS += -DDEBUG=yes
CFLAGS   += -O2 -g
LDFLAGS  += -g
else
CFLAGS += -Os -fomit-frame-pointer
endif

#-- variables
PACKAGE_CPPFLAGS += -DPACKAGE=\"$(PACKAGE)\"
PACKAGE_CPPFLAGS += -DVERSION=\"$(VERSION)\"
PACKAGE_CPPFLAGS += -DPREFIX=\"$(PREFIX)\"
PACKAGE_CPPFLAGS += -DPACKAGE_LOCALE_DIR=\"$(PREFIX)/share/locale\"

ifdef DESKTOP #environement variable to set if you want extra code included.
PACKAGE_CPPFLAGS += -DDESKTOP
endif

#-- GPE libs
ifndef CVSBUILD
CVBUILD = no
endif

ifeq ($(CVSBUILD),yes)
PACKAGE_CFLAGS  += -I../../libgpewidget
PACKAGE_LDFLAGS += -L../../libgpewidget
endif

LDFLAGS += -lgpewidget #need to be before gtk libs (overwrite things)

#-- GTK2
PACKAGE_CFLAGS  +=  #-DGTK_ENABLE_BROKEN
PACKAGE_CFLAGS  += `pkg-config --cflags gtk+-2.0 gdk-pixbuf-2.0`
PACKAGE_LDFLAGS += `pkg-config --libs   gtk+-2.0 gdk-pixbuf-2.0`

#-- SQLite
PACKAGE_LDFLAGS += -lsqlite

#.SUFFIXES: .d

default: $(PACKAGE)

ARM_BIN = $(PACKAGE)-$(VERSION)-arm-striped
arm: $(ARM_BIN)

.c.o:;
	$(CC) $(CFLAGS) $(CPPFLAGS) $(PACKAGE_CFLAGS) $(PACKAGE_CPPFLAGS) -c $< -o $@

#--package
$(PACKAGE): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS) $(PACKAGE_LDFLAGS)

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

