PACKAGE = gpe-ownerinfo
VERSION = 0.28
PREFIX = /usr/local
DEBUG = no
CVSBUILD = yes
ENABLE_NLS = yes

LINGUAS = de hu id pt pt_BR fr zh_TW ro sv nl sk cs ru sr es zh_CN vi fi tr sl ko

LIB_TARGET = libgpe-ownerinfo.a
LIB_MEMBERS = $(PACKAGE)
MEMBERS = main

PACKAGE_CPPFLAGS += $(STANDARD_CPPFLAGS) -DPACKAGE=\"$(PACKAGE)\" -DPREFIX=\"$(PREFIX)\" -DPACKAGE_LOCALE_DIR=\"$(PREFIX)/share/locale\"
PACKAGE_CFLAGS += $(STANDARD_CFLAGS) $(GPECFLAGS)
PACKAGE_LDFLAGS += $(STANDARD_LDFLAGS) $(GPELIBS)

ifeq ($(CVSBUILD),yes)
BUILD = ../build
else
BUILD = build
endif

ifeq ($(ENABLE_NLS),yes)
PACKAGE_CFLAGS += -DENABLE_NLS
endif


.SUFFIXES: .d
.PHONY: all-mo install-mo update-po extract-po clean-po translation-package dist dist-prep ipkg dpkg printinfo tag

OBJS = $(patsubst %,%.o,$(MEMBERS))
SOURCES = $(patsubst %,%.c,$(MEMBERS) $(LIB_MEMBERS))
LIB_OBJS = $(patsubst %,%.o,$(LIB_MEMBERS))
LIB_SOURCES = $(patsubst %,%.c,$(LIB_MEMBERS))

DEPS = $(patsubst %,%.d,$(MEMBERS) $(LIB_MEMBERS))

all: $(PACKAGE) all-mo

$(LIB_TARGET): $(LIB_OBJS)
	rm -f $@
	ar cq $@ $^

$(PACKAGE): $(OBJS) $(LIB_TARGET)
	$(CC) -o $@ $^ $(LDFLAGS) $(PACKAGE_LDFLAGS) -L. -lgpe-ownerinfo

install-program: all 
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	install -s $(PACKAGE) $(DESTDIR)$(PREFIX)/bin/
	install -d $(DESTDIR)/etc/gpe/

install-devel: 
	install -d $(DESTDIR)/$(PREFIX)/include
	install -m644 gpe-ownerinfo.h $(DESTDIR)/$(PREFIX)/include/gpe-ownerinfo.h
	install -d $(DESTDIR)/$(PREFIX)/lib
	install -m755 $(LIB_TARGET) $(DESTDIR)/$(PREFIX)/lib/$(LIB_TARGET)

clean: clean-po
	rm -f $(PACKAGE) $(OBJS) $(DEPS) $(LIB_OBJS) $(LIB_TARGET)

distclean: clean
	rm -rf familiar/dist/

include $(BUILD)/Makefile.translation
include $(BUILD)/Makefile.dpkg_ipkg

-include $(DEPS)
