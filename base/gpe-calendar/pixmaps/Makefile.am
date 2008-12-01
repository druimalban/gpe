# Install 48x48 icon into /usr/share/pixmaps on all systems
pixmapsdir = $(datadir)/pixmaps

pixmaps_DATA = gpe-calendar.png

gpe-calendar.png: gpe-calendar-48x48.png
	cp $< $@

EXTRA_DIST = $(pixmaps_DATA)

if HILDON

iconsdir = $(datadir)/icons/hicolor

#nobase_icons_DATA = 26x26/hildon/gpe-calendar.png 40x40/hildon/gpe-calendar.png 48x48/hildon/gpe-calendar.png 64x64/hildon/gpe-calendar.png scalable/hildon/gpe-calendar.png
nobase_icons_DATA = 26x26/hildon/gpe-calendar.png scalable/hildon/gpe-calendar.png

26x26/hildon/gpe-calendar.png: gpe-calendar-26x26.png
	-mkdir -p 26x26/hildon
	cp $< $@

40x40/hildon/gpe-calendar.png: gpe-calendar-40x40.png
	-mkdir -p 40x40/hildon
	cp $< $@

48x48/hildon/gpe-calendar.png: gpe-calendar-48x48.png
	-mkdir -p 48x48/hildon
	cp $< $@

64x64/hildon/gpe-calendar.png: gpe-calendar-64x64.png
	-mkdir -p 64x64/hildon
	cp $< $@

scalable/hildon/gpe-calendar.png: gpe-calendar-64x64.png
	-mkdir -p scalable/hildon
	cp $< $@

EXTRA_DIST += $(nobase_icons_DATA)

endif