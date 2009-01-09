desktopdir = $(datadir)/applications

if HILDON
desktop_in_files =

#nobase_icons_DATA = 26x26/hildon/bass.png 40x40/hildon/bass.png 48x48/hildon/bass.png 64x64/hildon/bass.png scalable/hildon/bass.png
nobase_icons_DATA = 26x26/hildon/bass.png scalable/hildon/bass.png
else
desktop_in_files = starling.desktop.in

nobase_icons_DATA = 
endif

desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

pixmapdir = $(datadir)/pixmaps
dist_pixmap_DATA = bass.png bass-20x20.png bass-26x26.png bass-40x40.png \
	bass-48x48.png bass-64x64.png

bass.png: bass-20x20.png
	ln -sf $< $@

iconsdir = $(datadir)/icons/hicolor

26x26/hildon/bass.png: bass-26x26.png
	-mkdir -p 26x26/hildon
	cp $< $@

40x40/hildon/bass.png: bass-40x40.png
	-mkdir -p 40x40/hildon
	cp $< $@

48x48/hildon/bass.png: bass-48x48.png
	-mkdir -p 48x48/hildon
	cp $< $@

64x64/hildon/bass.png: bass-64x64.png
	-mkdir -p 64x64/hildon
	cp $< $@

scalable/hildon/bass.png: bass-64x64.png
	-mkdir -p scalable/hildon
	cp $< $@

EXTRA_DIST = starling.desktop.in.in \
	     $(pixmaps_DATA) \
	     $(nobase_icons_DATA)

DISTCLEANFILES = $(desktop_DATA)