PACKAGE = gpe-today
PREFIX = /usr/local
VERSION = 0.09
CVSBUILD = yes
#DEBUG = yes
#EFENCE = yes

LINGUAS = vi tr es sr rw fi pt_BR pt de zh_CN zh_TW ga ro wa fr sv nl cs sk ru

MEMBERS = main todo calendar date xsettings

ifeq ($(EFENCE),yes)
PACKAGE_LDFLAGS = -lefence
endif

ifeq ($(DEBUG),yes)
package_CFLAGS = -g
else
PACKAGE_CFLAGS = -Os -fomit-frame-pointer
endif

.SUFFIXES: .d

ifeq ($(CVSBUILD),yes)
PACKAGE_CPPFLAGS += -I../libeventdb -I../libtododb -I../libxsettings -I../libxsettings-client -I../libdisplaymigration -I../libgpepimc
PACKAGE_LDFLAGS += -L../libeventdb -L../libtododb -L../libxsettings-client -Wl,-rpath-link,../libxsettings -L../libgpepimc
endif
PACKAGE_CPPFLAGS += $(STANDARD_CPPFLAGS)
PACKAGE_CFLAGS += $(STANDARD_CFLAGS) $(GPECFLAGS)
PACKAGE_LDFLAGS += $(STANDARD_LDFLAGS) $(GPELIBS) -leventdb -ltododb -lXsettings-client -lsqlite -lgpepimc
   

OBJS = $(patsubst %,%.o,$(MEMBERS))
DEPS = $(patsubst %,%.d,$(MEMBERS))
SOURCES = $(patsubst %,%.c,$(MEMBERS))

PIXMAPS = calendar.png date.png tasks.png

ifeq ($(CVSBUILD),yes)
BUILD = ../build
else
BUILD = build
endif

all: $(PACKAGE)

$(PACKAGE): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS) $(PACKAGE_LDFLAGS)

install-program: all $(PACKAGE).desktop
	for i in $(PIXMAPS); do install -D -m 644 pixmaps/$$i $(DESTDIR)$(PREFIX)/share/$(PACKAGE)/pixmaps/$$i; done
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	install -s $(PACKAGE) $(DESTDIR)$(PREFIX)/bin/$(PACKAGE)
	mkdir -p $(DESTDIR)$(PREFIX)/share/pixmaps
	install -m 644 $(PACKAGE).png $(DESTDIR)$(PREFIX)/share/pixmaps/$(PACKAGE).png
	mkdir -p $(DESTDIR)$(PREFIX)/share/applications
	install -m 644 $(PACKAGE).desktop $(DESTDIR)$(PREFIX)/share/applications/$(PACKAGE).desktop
	install -m 644 -D gtkrc $(DESTDIR)$(PREFIX)/share/$(PACKAGE)/gtkrc

clean: clean-po
	rm -f $(PACKAGE) $(OBJS) $(DEPS) $(PACKAGE).desktop *~
	rm -rf familiar/dist

mrproper: clean
	rm -f *.ipk translation-ipkgs.txt
	rm -rf familiar/dist-translation

include $(BUILD)/Makefile.translation
include $(BUILD)/Makefile.dpkg_ipkg
-include $(DEPS)
