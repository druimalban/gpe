PREFIX = /usr/local
PACKAGE = gpe-contacts
GTK2 = yes
pkgdatadir = $(PREFIX)/share/$(PACKAGE)
CVSBUILD = yes
STRIP = strip
VERSION = 0.27
DEBUG = no
MYPIXMAPSDIR = $(PREFIX)/share/gpe/pixmaps/default/$(PACKAGE)

LINGUAS = de sv cs nl hu ro ru sr sk es

ifeq ($(CVSBUILD),yes)
BUILD = ../build
else
BUILD = build
endif

ifeq ($(CVSBUILD),yes)
PACKAGE_CPPFLAGS += -I../libdisplaymigration -I../libgpepimc -I../libgpevtype
PACKAGE_LDFLAGS += -L../libdisplaymigration -L../libgpepimc -L../libgpevtype
endif
PACKAGE_CPPFLAGS += $(STANDARD_CPPFLAGS) -DENABLE_NLS -DDBUS_API_SUBJECT_TO_CHANGE
PACKAGE_CPPFLAGS += -DPACKAGE=\"$(PACKAGE)\" -DPREFIX=\"$(PREFIX)\" -DPACKAGE_LOCALE_DIR=\"$(PREFIX)/share/locale\"
PACKAGE_CFLAGS += $(STANDARD_CFLAGS) $(GPECFLAGS) `pkg-config --cflags libxml-2.0` `pkg-config --cflags mimedir-1.0` `pkg-config --cflags dbus-glib-1`
PACKAGE_LDFLAGS += $(STANDARD_LDFLAGS) $(GPELIBS) -lsqlite `pkg-config --libs libxml-2.0` `pkg-config --libs mimedir-1.0` `pkg-config --libs dbus-glib-1` -ldisplaymigration -lgpepimc -lgpevtype

MEMBERS = db editing main structure support configure old-db export namedetail import-vcard

DEPS = $(patsubst %,%.d,$(MEMBERS))
OBJS = $(patsubst %,%.o,$(MEMBERS))
SOURCES = $(patsubst %,%.c,$(MEMBERS))

all: gpe-contacts all-mo

$(PACKAGE): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS) $(PACKAGE_LDFLAGS)

clean:
	rm -f gpe-contacts $(OBJS) $(DEPS)

install-program: all $(PACKAGE).desktop
	install -d $(DESTDIR)$(PREFIX)/bin
	install  gpe-contacts $(DESTDIR)$(PREFIX)/bin/gpe-contacts
	$(STRIP) $(DESTDIR)$(PREFIX)/bin/gpe-contacts
	install -d $(DESTDIR)$(PREFIX)/share/applications
	install -m 644 gpe-contacts.desktop $(DESTDIR)$(PREFIX)/share/applications/
	install -d $(DESTDIR)$(PREFIX)/share/pixmaps
	install -m 644 icons/gpe-contacts.png $(DESTDIR)$(PREFIX)/share/pixmaps/gpe-contacts.png
	install -d $(DESTDIR)$(pkgdatadir)
	install -m 644 contacts-layout.xml $(DESTDIR)$(pkgdatadir)/default-layout.xml
	install -m 644 contacts-layout-bigscreen.xml $(DESTDIR)$(pkgdatadir)/contacts-layout-bigscreen.xml
	install -m 644 well-known-tags $(DESTDIR)$(pkgdatadir)/well-known-tags
	install -d $(DESTDIR)$(MYPIXMAPSDIR)
	install -m 644 icons/frame.png $(DESTDIR)$(MYPIXMAPSDIR)/frame.png
	install -m 644 icons/entry.png $(DESTDIR)$(MYPIXMAPSDIR)/entry.png
	install -m 644 icons/notebook.png $(DESTDIR)$(MYPIXMAPSDIR)/notebook.png
	install -m 644 icons/export.png $(DESTDIR)$(MYPIXMAPSDIR)/export.png

include $(BUILD)/Makefile.dpkg_ipkg
include $(BUILD)/Makefile.translation
-include $(DEPS)
