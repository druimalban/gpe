INCLUDES = -DPKGDATADIR=\"$(pkgdatadir)\" \
           -DG_LOG_DOMAIN=\"gpe-scap\" \
	   -DGPESCAPLOCALEDIR=\""$(gpescaplocaledir)"\" \
           -I$(top_srcdir) \
	   $(GTK_CFLAGS) \
	   $(GLADE_CFLAGS) \
       -D_GNU_SOURCE

LIBS = $(GTK_LIBS) $(GLADE_LIBS) $(XSHAPE_LIBS)

bin_PROGRAMS = gpe-scap
gpe_scap_SOURCES = scr-i18n.h \
			 glade-utils.c \
			 glade-utils.h \
			 scr-shot.h \
			 scr-shot-common.c \
			 screenshot-utils.c \
			 screenshot-utils.h \
			 scr-shot-x.c \
			 gpe-scap.c \
             cfgfile.h \
             cfgfile.c

pkgdata_DATA = gpe-scap.glade

EXTRA_DIST = $(pkgdata_SCRIPTS) $(pkgdata_DATA) gpe-scap.gladep
