INCLUDES = -DPKGDATADIR=\"$(pkgdatadir)\" \
           -DG_LOG_DOMAIN=\"gpe-screenshot\" \
	   -DGPESCREENSHOTLOCALEDIR=\""$(gpescreenshotlocaledir)"\" \
           -I$(top_srcdir) \
	   $(GTK_CFLAGS) \
	   $(GLADE_CFLAGS)

LIBS = $(GTK_LIBS) $(GLADE_LIBS) $(XSHAPE_LIBS)

bin_PROGRAMS = gpe-screenshot
gpe_screenshot_SOURCES = scr-i18n.h \
			 glade-utils.c \
			 glade-utils.h \
			 scr-shot.h \
			 scr-shot-common.c \
			 screenshot-utils.c \
			 screenshot-utils.h \
			 scr-shot-x.c \
			 gpe-screenshot.c

pkgdata_SCRIPTS = scap-png-upload

pkgdata_DATA = gpe-screenshot.glade

EXTRA_DIST = $(pkgdata_SCRIPTS) $(pkgdata_DATA) gpe-screenshot.gladep
