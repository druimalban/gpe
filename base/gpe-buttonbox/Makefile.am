bin_PROGRAMS = gpe-buttonbox gpe-buttonbox-config

gpe_buttonbox_SOURCES = gpe-buttonbox.c system-tray.c cfgfile.c cfgfile.h globals.h

gpe_buttonbox_config_SOURCES = gpe-buttonbox-config.c cfgfile.c cfgfile.h globals.h

INCLUDES = @BUTTONBOX_CFLAGS@ -DPREFIX=\"@prefix@\" -D_GNU_SOURCE -Wall
           
LDADD = @BUTTONBOX_LIBS@

desktopdir = $(datadir)/applications
desktop_in_files = apps/gpe-buttonbox.desktop.in \
                   apps/gpe-buttonbox-config.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

pixmapsdir = $(datadir)/pixmaps/gpe/default/gpe-buttonbox/

pixmaps_DATA = pixmaps/show-desktop.png \
               pixmaps/other-app.png \
			   pixmaps/docs.png \
			   pixmaps/blue_button_down.png \
               pixmaps/yellow_button_down.png \
               pixmaps/blue_button_up.png pixmaps/yellow_button_up.png \
               pixmaps/separator.png 

configdir = $(sysconfdir)/gpe
config_DATA = button-box.apps

EXTRA_DIST = apps/gpe-buttonbox.desktop.in.in \
             apps/gpe-buttonbox-config.desktop.in.in \
             $(pixmaps_DATA) \
             $(config_DATA) \
             intltool-extract.in \
             intltool-merge.in \
             intltool-update.in \
             po/POTFILES.in

DISTCLEANFILES = intltool-extract \
                 intltool-merge \
                 intltool-update \
                 po/POTFILES \
                 po/.intltool-merge-cache \
                 apps/gpe-buttonbox.desktop \
                 apps/gpe-buttonbox-config.desktop

include gpe-dist.am
