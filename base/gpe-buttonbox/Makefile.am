bin_PROGRAMS = gpe-buttonbox gpe-buttonbox-config

gpe_buttonbox_SOURCES = gpe-buttonbox.c system-tray.c

INCLUDES = @BUTTONBOX_CFLAGS@ -DPREFIX=\"@prefix@\" -D_GNU_SOURCE -Wall
           
LDADD = @BUTTONBOX_LIBS@

EXTRA_DIST = show-desktop.png other-app.png

install-data-local:
	mkdir -p $(DESTDIR)$(datadir)/gpe/pixmaps/default/gpe-buttonbox
	install -m 0644 show-desktop.png $(DESTDIR)$(datadir)/gpe/pixmaps/default/gpe-buttonbox/
	install -m 0644 other-app.png $(DESTDIR)$(datadir)/gpe/pixmaps/default/gpe-buttonbox/
	install -m 0644 docs.png $(DESTDIR)$(datadir)/gpe/pixmaps/default/gpe-buttonbox/
	for i in blue_button_down yellow_button_down blue_button_up yellow_button_up separator; do install -m 0644 pixmaps/$$i.png $(DESTDIR)$(datadir)/gpe/pixmaps/default/gpe-buttonbox/$$i.png; done
	install -d $(DESTDIR)/etc/gpe
	install -m 0644 button-box.apps $(DESTDIR)/etc/gpe/

#	install -d $(DESTDIR)$(datadir)/gpe/overrides
#	for i in abiword balsa gnumeric mozilla-firefox gpe-contacts gpe-calendar galculator gaim gpe-edit gpe-filemanager gpe-todo gkdial gpdf gpe-terminal gpe-gallery gpe-sketchbook totem gpe-mixer gpe-conf sol edit-vcard; do install -m 0644 apps/$$i.desktop $(DESTDIR)$(datadir)/gpe/overrides/$$i.desktop; done
#	for i in audio_mixer calculator calendar contacts control_panels email file_manager image_gallery instant_messenger media_player modern_dialer pdf_viewer sketch_book sheet terminal text_editor to_do_list web word aisleriot_solitaire Printer edit-vcard gpe-config-ownerinfo gpe-config-serial gpe-config-sound gpe-config-users gpe-package gpe-shield; do install -m 0644 apps/$$i.png $(DESTDIR)$(datadir)/gpe/overrides/$$i.png; done

	install -d $(DESTDIR)$(datadir)/applications
	install -m 0644 apps/gpe-buttonbox-config.desktop $(DESTDIR)$(datadir)/applications/

include gpe-dist.am
