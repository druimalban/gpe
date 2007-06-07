desktopdir = $(datadir)/applications

if HILDON
desktop_in_files =
else
desktop_in_files = starling.desktop.in
endif

desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

pixmapdir = $(datadir)/pixmaps
pixmap_DATA = bass.png

EXTRA_DIST = starling.desktop.in.in bass.png

DISTCLEANFILES = $(desktop_DATA)