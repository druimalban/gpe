desktopdir = $(datadir)/applications/hildon
desktop_in_files = gpe-contacts.desktop.in

servicedir = $(datadir)/dbus-1/services
service_in_files = com.nokia.gpe-contacts.service.in

desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
service_DATA = $(service_in_files:.service.in=.service)
@INTLTOOL_DESKTOP_RULE@

metadir = $(datadir)/gpe-contacts
meta_DATA = default-layout-bigscreen.xml

pixmapsdir = $(datadir)/pixmaps
# we need a 26x26 icon here 
pixmaps_DATA = gpe-contacts.png

gpe-contacts.png: gpe-contacts-48x48.png
	cp $< $@

iconsdir = $(datadir)/icons/hicolor

#nobase_icons_DATA = 26x26/hildon/gpe-contacts.png 40x40/hildon/gpe-contacts.png 48x48/hildon/gpe-contacts.png 64x64/hildon/gpe-contacts.png scalable/hildon/gpe-contacts.png
nobase_icons_DATA = 26x26/hildon/gpe-contacts.png scalable/hildon/gpe-contacts.png

26x26/hildon/gpe-contacts.png: gpe-contacts-26x26.png
	-mkdir -p 26x26/hildon
	cp $< $@

40x40/hildon/gpe-contacts.png: gpe-contacts-40x40.png
	-mkdir -p 40x40/hildon
	cp $< $@

48x48/hildon/gpe-contacts.png: gpe-contacts-48x48.png
	-mkdir -p 48x48/hildon
	cp $< $@

64x64/hildon/gpe-contacts.png: gpe-contacts-64x64.png
	-mkdir -p 64x64/hildon
	cp $< $@

scalable/hildon/gpe-contacts.png: gpe-contacts-64x64.png
	-mkdir -p scalable/hildon
	cp $< $@


EXTRA_DIST = $(service_DATA) \
	     $(desktop_DATA) \
	     $(pixmaps_DATA) \
	     $(meta_DATA) \
	     $(nobase_icons_DATA)
             
