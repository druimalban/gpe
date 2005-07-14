INCLUDES = -DGLADE_XML_PREFIX=\"$(datadir)/gpe-mileage\" \
           -DG_LOG_DOMAIN=\"gpe-mileage\" \
	   -DGPEMILEAGELOCALEDIR=\""$(gpemileagelocaledir)"\" \
           -I$(top_srcdir) \
	   $(GTK_CFLAGS) \
	   $(GLADE_CFLAGS) \
	   $(SQLITE_CFLAGS)

LIBS = $(GTK_LIBS) $(GLADE_LIBS) $(SQLITE_LIBS)

bin_PROGRAMS = gpe-mileage
gpe_mileage_SOURCES = car.c \
		      car.h \
		      fuel_item.c \
		      fuel_item.h \
		      gconf-utility.c \
		      gconf-utility.h \
		      gpe-mileage-i18n.h \
		      gui.c \
		      gui.h \
		      main.c \
		      main_window.c \
		      main_window.h \
		      mileage_db.c \
		      mileage_db.h \
		      new_fuel_item_dlg.c \
		      new_fuel_item_dlg.h \
		      preferences.c \
		      preferences.h \
		      preferences_dlg.c \
		      preferences_dlg.h \
		      units.c \
		      units.h \
		      view-common.h

EXTRA_DIST = gpe-mileage.glade gpe-mileage.gladep

gladefilesdir = $(pkgdatadir)
gladefiles_DATA = gpe-mileage.glade
