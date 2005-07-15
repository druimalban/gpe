INCLUDES = -DGLADE_XML_PREFIX=\"$(datadir)/gpe-life\" \
           -DG_LOG_DOMAIN=\"gpe-life\" \
	   -DGPEMILEAGELOCALEDIR=\""$(gpelifelocaledir)"\" \
           -I$(top_srcdir) \
	   $(GTK_CFLAGS) \
	   $(GLADE_CFLAGS)

LIBS = $(GTK_LIBS) $(GLADE_LIBS)

bin_PROGRAMS = gpe-life
gpe_life_SOURCES = gpe-life-i18n.h \
		   gui.c \
		   gui.h \
		   l-cell.c \
		   l-cell.h \
		   l-grid.c \
		   l-grid.h \
		   l-model.c \
		   l-model.h \
		   l-view.c \
		   l-view.h \
		   main.c

EXTRA_DIST = gpe-life.glade gpe-life.gladep

gladefilesdir = $(pkgdatadir)
gladefiles_DATA = gpe-life.glade
