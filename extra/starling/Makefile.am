SUBDIRS = data src po
if HILDON
SUBDIRS += hildon
endif

EXTRA_DIST = intltool-extract.in \
             intltool-merge.in \
             intltool-update.in \
	     HACKING TODO

DISTCLEANFILES = intltool-extract \
		intltool-merge \
		intltool-update

clean-local:
	$(RM) po/.intltool-merge-cache
