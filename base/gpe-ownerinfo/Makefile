PACKAGE = gpe-ownerinfo
VERSION = 0.21

PREFIX = /usr/local
DEBUG = no
CVSBUILD = yes
LINGUAS = de hu id pt fr zh_TW ro sv nl sk cs

LIB_TARGET = libgpe-ownerinfo.a
LIB_MEMBERS = $(PACKAGE)
MEMBERS = main

ifeq ($(CVSBUILD),yes)
CFLAGS += -I../libgpewidget
LDFLAGS += -L../libgpewidget
BUILD = ../build
else
CFLAGS += -I/usr/include/gpe
BUILD = build
endif
LDFLAGS += -lgpewidget

GTKCFLAGS = `pkg-config --cflags gtk+-2.0`
GTKLDFLAGS += `pkg-config --libs gtk+-2.0`

CPPFLAGS = $(GTKCFLAGS) -D_GNU_SOURCE 
ifeq ($(DEBUG),yes)
CFLAGS += -O2 -g
LDFLAGS = -g #-lefence
else
CFLAGS += -Os -fomit-frame-pointer
endif
CFLAGS += -Wall
CPPFLAGS += -DPACKAGE=\"$(PACKAGE)\" -DPREFIX=\"$(PREFIX)\" -DPACKAGE_LOCALE_DIR=\"$(PREFIX)/share/locale\"
CFLAGS += -MD

.SUFFIXES: .d
.PHONY: all-mo install-mo update-po extract-po clean-po translation-package dist dist-prep ipkg dpkg printinfo tag

OBJS = $(patsubst %,%.o,$(MEMBERS))
SOURCES = $(patsubst %,%.c,$(MEMBERS))
LIB_OBJS = $(patsubst %,%.o,$(LIB_MEMBERS))
LIB_SOURCES = $(patsubst %,%.c,$(LIB_MEMBERS))

DEPS = $(patsubst %,%.d,$(MEMBERS) $(LIB_MEMBERS))

all: $(PACKAGE) all-mo

$(LIB_TARGET): $(LIB_OBJS)
	rm -f $@
	ar cq $@ $^

$(PACKAGE): $(OBJS) $(LIB_TARGET)
	$(CC) -o $@ $^  $(GTKLDFLAGS) $(LDFLAGS) -L. -lgpe-ownerinfo

install-program: all 
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	install -s $(PACKAGE) $(DESTDIR)$(PREFIX)/bin/
	install -d $(DESTDIR)/etc/gpe/

clean: clean-po
	rm -f $(PACKAGE) $(OBJS) $(DEPS)

distclean: clean
	rm -rf familiar/dist/

include $(BUILD)/Makefile.translation
include $(BUILD)/Makefile.dpkg_ipkg

-include $(DEPS)
