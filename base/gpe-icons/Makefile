PREFIX = /usr/local
PACKAGE = gpe-icons
PIXMAPS = copy.png cut.png exit.png new.png open.png paste.png save.png save_as.png home.png future_view.png day_view.png week_view.png month_view.png year_view.png cancel.png empty.png properties.png search.png search_and_replace.png delete.png ok.png error.png lock.png lock16.png clock.png stop_clock.png dir-open.png dir-closed.png dir-up.png media-play.png media-pause.png media-stop.png media-fwd.png media-rew.png media-next.png media-prev.png media-eject.png what.png clean.png hide.png help.png question.png about.png left.png right.png highscores.png stop.png preferences.png up.png refresh.png tux-48.png bell.png bell_recur.png recur.png edit.png warning16.png tick.png cellphone.png network.png
MONOCHROME = no

# these are icons that are in CVS for completeness but are not actually used anywhere yet:
# warning48.png

ifeq ($(MONOCHROME),yes)
PIXMAPS_DIR = default-monochrome
else
PIXMAPS_DIR = default
endif

all:

PIXMAPS_DIR = default

install:
	for i in $(PIXMAPS); do install -m 644 -D $(PIXMAPS_DIR)/$$i $(DESTDIR)$(PREFIX)/share/gpe/pixmaps/default/$$i; done
	install -D -m 644 gpe-logo.png $(DESTDIR)$(PREFIX)/share/gpe/pixmaps/gpe-logo.png

clean:

upstream: clean
	( VERSION=`dpkg-parsechangelog | grep ^Version | cut -d\  -f2 | cut -d- -f1`; cd ..; cp -a $(PACKAGE) $(PACKAGE)-$$VERSION.orig; tar czf $(PACKAGE)_$$VERSION.orig.tar.gz --exclude=$(PACKAGE)-$$VERSION.orig/debian --exclude=$(PACKAGE)-$$VERSION.orig/CVS --exclude=$(PACKAGE)-$$VERSION.orig/familiar --exclude=$(PACKAGE)-$$VERSION.orig/default/CVS $(PACKAGE)-$$VERSION.orig ; rm -rf $(PACKAGE)-$$VERSION.orig )

dpkg: upstream
	( VERSION=`dpkg-parsechangelog | grep ^Version | cut -d\  -f2 | cut -d- -f1`; TMP=/var/tmp/debian/$(PACKAGE); PV=$(PACKAGE)-$$VERSION; rm -rf $$TMP; mkdir -p $$TMP; cp ../$(PACKAGE)_$$VERSION.orig.tar.gz $$TMP; (cd $$TMP; tar xzf $(PACKAGE)_$$VERSION.orig.tar.gz ; mv $$PV.orig $$PV ); mkdir -p $$TMP/$$PV/debian; cp debian/* $$TMP/$$PV/debian; cd $$TMP/$$PV; fakeroot dpkg-buildpackage  ; cd ..; rm -rf $$PV)

ipkg:
	rm -rf familiar/dist
	mkdir -p familiar/dist/CONTROL
	cp familiar/control familiar/dist/CONTROL/control
	make DESTDIR=`pwd`/familiar/dist PREFIX=/usr install
	chown -R root.root familiar/dist
	ipkg-build familiar/dist
