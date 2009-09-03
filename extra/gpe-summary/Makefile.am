# home applet

SUBDIRS = po


libsbdir = $(libdir)/$(APPLETDIR)
datsbdir = $(datadir)/applications/hildon-home
libsb_LTLIBRARIES = libgpesummary.la
datsb_DATA = homegpesummary.desktop

homegpesummary.desktop : $(DESKTOPFILE)
	cp $(DESKTOPFILE) homegpesummary.desktop

libgpesummary_la_SOURCES = gpesummary.c
libgpesummary_la_CFLAGS = $(DEPS_CFLAGS) $(HILDON_CFLAGS)
#`pkg-config --cflags --libs gtk+-2.0 hildon-libs dbus-1 libosso libeventdb libtododb libcontactsdb`
libgpesummary_la_LDFLAGS = -module -avoid-version $(DEPS_LIBS) $(HILDON_LIBS)

EXTRA_DIST =					\
        autogen.sh				\
	intltool-extract.in 			\
	intltool-merge.in   			\
	intltool-update.in  			\
        debian/changelog			\
        debian/compat				\
        debian/copyright			\
        debian/control				\
        debian/rules				\
	homegpesummary.hdhomepluginitem		\
	homegpesummary.hildonhomeapplet
