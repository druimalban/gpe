
SUBDIRS = po

INCLUDES = $(DEPS_CFLAGS) -I$(top_srcdir) \
           -DPREFIX=\"@prefix@\" -D_GNU_SOURCE \
	   -DPACKAGE_LOCALE_DIR=\"@gpeirc_localedir@\"

bin_PROGRAMS = gpe-irc

gpe_irc_LDADD = @DEPS_LIBS@

gpe_irc_SOURCES = \
		main.c irc.c irc_input.c \
		dictionary.c general_config.c general_config_sql.c \
		networks_config.c networks_config_sql.c \
		irc_parse.c ctcp.c \
		main.h irc.h irc_input.h \
		dictionary.h general_config.h general_config_sql.h \
		networks_config.h networks_config_sql.h \
		irc_parse.h irc_reply.h ctcp.h 

desktopdir = $(datadir)/applications
desktop_in_files = gpe-irc.desktop.in

desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

pixmapsdir = $(datadir)/pixmaps
pixmaps_DATA =	gpe-irc.png

pixmapsextradir = $(datadir)/pixmaps/default/irc/
pixmapsextra_DATA = \
		pixmaps/globe.png \
		pixmaps/quote.png

pixmapssmileysdir = $(datadir)/gpe/pixmaps/default/irc/smileys
pixmapssmileys_DATA = pixmaps/smileys/happy.png

pixmapscolorsdir = $(datadir)/gpe/pixmaps/default/irc/colors
pixmapscolors_DATA =	\
		pixmaps/colors/black.png \
		pixmaps/colors/blue.png \
		pixmaps/colors/green.png \
		pixmaps/colors/red.png \
		pixmaps/colors/white.png

EXTRA_DIST = $(desktop_DATA) \
             $(pixmaps_DATA) \
             $(pixmapsextra_DATA) \
             $(pixmapssmileys_DATA) \
             $(pixmapscolors_DATA) \
             $(help_DATA) \
             intltool-extract.in \
             intltool-merge.in \
             intltool-update.in
			 

DISTCLEANFILES = intltool-extract \
                 intltool-merge \
                 intltool-update

clean-local:
	rm -f gpe-irc.desktop
	rm -f po/.intltool-merge-cache
	rm -rf autom4te.cache
	rm -f *.d *~

deb: dist
	-mkdir $(top_builddir)/debian-build
	cd $(top_builddir)/debian-build && tar xzf $(top_builddir)/../$(PACKAGE)-$(VERSION).tar.gz
	cp -r $(top_builddir)/debian $(top_builddir)/debian-build/$(PACKAGE)-$(VERSION)
	cd $(top_builddir)/debian-build/$(PACKAGE)-$(VERSION) && dpkg-buildpackage -rfakeroot
	-rm -rf $(top_builddir)/debian-build/$(PACKAGE)-$(VERSION)
	
include gpe-dist.am
include gpe-tp.am
