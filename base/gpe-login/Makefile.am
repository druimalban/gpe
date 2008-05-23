
SUBDIRS = po

INCLUDES = $(DEPS_CFLAGS) -I$(top_srcdir) \
           -DPREFIX=\"@prefix@\" -D_GNU_SOURCE \
           -DPACKAGE_LOCALE_DIR=\"@gpelogin_localedir@\"

bin_PROGRAMS = gpe-login gpe-logout gpe-lock-display

gpe_login_LDADD = @DEPS_LIBS@ -lgpe-ownerinfo -lcrypt
gpe_login_SOURCES = gpe-login.c

gpe_logout_LDADD = @DEPS_LIBS@
gpe_logout_SOURCES = gpe-logout.c

gpe_lock_display_LDADD = @DEPS_LOCK_LIBS@
gpe_lock_display_SOURCES = gpe-lock-display.c

x11dir = $(sysconfdir)/X11
x11_SCRIPTS = gpe-login.setup X11/gpe-login.pre-session
x11_DATA = gpe-login.keylaunchrc X11/gpe-login.gtkrc

apmdir = $(sysconfdir)/apm/suspend.d
apm_SCRIPTS = gpe-login.suspend 
	
xsessiondir = $(sysconfdir)/X11/Xsession.d
xsession_SCRIPTS = gpe-login.session

xinitdir = $(sysconfdir)/X11/Xinit.d
xinit_SCRIPTS = gpe-login.xinit

gpedir = $(sysconfdir)/gpe
gpe_DATA = gpe-login.conf locale.alias

sysdir = $(sysconfdir)/sysconfig
sys_DATA = gpe-login.sysconfig

pixmapsdir = $(datadir)/gpe/pixmaps/default
pixmaps_DATA = \
               pixmaps/session-back.png \
               pixmaps/session-lock.png \
               pixmaps/session-halt.png \
               pixmaps/session-logout.png \
               pixmaps/session-suspend.png


EXTRA_DIST = \
             $(x11_SCRIPTS) \
             $(x11_DATA) \
             $(apm_SCRIPTS) \
             $(xsession_SCRIPTS) \
             $(xinit_SCRIPTS) \
             $(gpe_DATA) \
             $(sys_DATA) \
             $(pixmaps_DATA) \
             intltool-extract.in \
             intltool-merge.in \
             intltool-update.in
			 

DISTCLEANFILES = intltool-extract \
                 intltool-merge \
                 intltool-update

install-data-hook:
	cd $(DESTDIR)$(apmdir) && mv gpe-login.suspend S98lock-display
	cd $(DESTDIR)$(xsessiondir) && mv gpe-login.session 50autolock
	cd $(DESTDIR)$(xinitdir) && mv gpe-login.xinit 99gpe-login
	cd $(DESTDIR)$(sysdir) && mv gpe-login.sysconfig gpelogin

uninstall-hook:
	$(RM) -f $(DESTDIR)$(apmdir)/S98lock-display
	$(RM) -f $(DESTDIR)$(xsessiondir)/50autolock
	$(RM) -f $(DESTDIR)$(xinitdir)/99gpe-login
	$(RM) -f $(DESTDIR)$(sysdir)/gpelogin
	
clean-local:
	rm -f *.d *~

deb: dist
	-mkdir $(top_builddir)/debian-build
	cd $(top_builddir)/debian-build && tar xzf $(top_builddir)/../$(PACKAGE)-$(VERSION).tar.gz
	cp -r $(top_builddir)/debian $(top_builddir)/debian-build/$(PACKAGE)-$(VERSION)
	cd $(top_builddir)/debian-build/$(PACKAGE)-$(VERSION) && dpkg-buildpackage -rfakeroot
	-rm -rf $(top_builddir)/debian-build/$(PACKAGE)-$(VERSION)
	
include gpe-dist.am
include gpe-tp.am
