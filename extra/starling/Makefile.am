SUBDIRS = data src po
if HILDON
SUBDIRS += hildon
endif

EXTRA_DIST = intltool-extract.in \
             intltool-merge.in \
             intltool-update.in \
	     HACKING TODO \
	     debian/changelog debian/compat debian/control debian/rules \
	     po/starling.pot 

DISTCLEANFILES = intltool-extract \
		intltool-merge \
		intltool-update

clean-local:
	$(RM) po/.intltool-merge-cache

include gpe-dist.am
