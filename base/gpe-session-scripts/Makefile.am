
SUBDIRS = po

binscriptdir = $(bindir)
binscript_SCRIPTS = gpe-logout.sh gpe-auto-bluetooth.sh

xsessiondir = $(sysconfdir)/X11/Xsession.d
xsession_SCRIPTS = X11/Xsession.d/98standard-apps \
                   X11/Xsession.d/99xWindowManager

gpedir = $(sysconfdir)/gpe
gpe_DATA = standard.mbdock X11/xsettings.default

desktopdir = $(datadir)/applications
desktop_in_files = gpe-logout.desktop.in

desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

pixmapsdir = $(datadir)/pixmaps
pixmaps_DATA = gpe-logout.png \
	       gpe-logout-menu.png

EXTRA_DIST = $(desktop_DATA) \
             $(pixmaps_DATA) \
             $(xsession_SCRIPTS) \
             $(binscript_SCRIPTS) \
             $(gpe_DATA) \
             intltool-extract.in \
             intltool-merge.in \
             intltool-update.in
			 
install-data-hook:
	cd $(DESTDIR)$(sysconfdir) && mv gpe/standard.mbdock gpe/gpe.mbdock
	cd $(DESTDIR)$(bindir) && mv gpe-auto-bluetooth.sh  gpe-auto-bluetooth
	
uninstall-hook:
	cd $(DESTDIR)$(sysconfdir) && $(RM) gpe/gpe.mbdock
	cd $(DESTDIR)$(bindir) && $(RM) gpe-auto-bluetooth


DISTCLEANFILES = intltool-extract \
                 intltool-merge \
                 intltool-update

clean-local:
	rm -f gpe-logout.desktop
	rm -f po/.intltool-merge-cache
	rm -rf autom4te.cache
	rm -f *.d *~

deb: dist
	-mkdir $(top_builddir)/debian-build
	cd $(top_builddir)/debian-build && tar xzf $(top_builddir)/../$(PACKAGE)-$(VERSION).tar.gz
	cp -r $(top_builddir)/debian $(top_builddir)/debian-build/$(PACKAGE)-$(VERSION)
	cd $(top_builddir)/debian-build/$(PACKAGE)-$(VERSION) && dpkg-buildpackage -rfakeroot
	-rm -rf $(top_builddir)/debian-build/$(PACKAGE)-$(VERSION)
	
include gpe-dist.am
include gpe-tp.am
