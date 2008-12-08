desktopdir = $(datadir)/applications

if HILDON
desktop_in_files =

#nobase_icons_DATA = 26x26/hildon/base.png 40x40/hildon/base.png 48x48/hildon/base.png 64x64/hildon/base.png scalable/hildon/base.png
nobase_icons_DATA = 26x26/hildon/base.png scalable/hildon/base.png
else
desktop_in_files = starling.desktop.in

nobase_icons_DATA = 
endif

desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

pixmapdir = $(datadir)/pixmaps
pixmap_DATA = bass.png

base.png: base-20x20.png
	cp $< $@

iconsdir = $(datadir)/icons/hicolor

26x26/hildon/base.png: base-26x26.png
	-mkdir -p 26x26/hildon
	cp $< $@

40x40/hildon/base.png: base-40x40.png
	-mkdir -p 40x40/hildon
	cp $< $@

48x48/hildon/base.png: base-48x48.png
	-mkdir -p 48x48/hildon
	cp $< $@

64x64/hildon/base.png: base-64x64.png
	-mkdir -p 64x64/hildon
	cp $< $@

scalable/hildon/base.png: base-64x64.png
	-mkdir -p scalable/hildon
	cp $< $@

EXTRA_DIST = starling.desktop.in.in \
	     $(pixmaps_DATA) \
	     $(nobase_icons_DATA)

DISTCLEANFILES = $(desktop_DATA)