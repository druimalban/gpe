PREFIX = /usr/local
PACKAGE = gpe-icons
PIXMAPS = close.png copy.png cut.png exit.png new.png open.png paste.png save.png save_as.png home.png future_view.png day_view.png week_view.png month_view.png year_view.png cancel.png empty.png properties.png search.png search_and_replace.png delete.png ok.png error.png lock.png lock16.png clock.png stop_clock.png dir-open.png dir-closed.png dir-up.png media-play.png media-pause.png media-stop.png media-fwd.png media-rew.png media-next.png media-prev.png media-eject.png what.png clean.png hide.png help.png question.png about.png left.png right.png highscores.png stop.png preferences.png up.png refresh.png tux-48.png bell.png bell_recur.png recur.png edit.png warning16.png tick.png bt-logo.png loading.png list-view.png icon-view.png clock-popup.png send_and_recieve.png
MONOCHROME = no
CVSBUILD = yes
VERSION = 0.23

# these are icons that are in CVS for completeness but are not actually used anywhere yet:
# warning48.png

ifeq ($(MONOCHROME),yes)
PIXMAPS_DIR = default-monochrome
else
PIXMAPS_DIR = default
endif

all:

PIXMAPS_DIR = default

install-program:
	for i in $(PIXMAPS); do install -m 644 -D $(PIXMAPS_DIR)/$$i $(DESTDIR)$(PREFIX)/share/gpe/pixmaps/default/$$i; done
	install -D -m 644 gpe-logo.png $(DESTDIR)$(PREFIX)/share/gpe/pixmaps/gpe-logo.png

clean:

ifeq ($(CVSBUILD),yes)
BUILD = ../build
else
BUILD = build
endif

include $(BUILD)/Makefile.dpkg_ipkg

