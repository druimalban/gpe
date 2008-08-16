desktopdir = $(datadir)/applications

desktop_in_files = gpe-memo.desktop.in

desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

pixmapdir = $(datadir)/pixmaps
pixmap_DATA = gpe-memo.png

EXTRA_DIST = gpe-memo.desktop.in.in gpe-memo.png

DISTCLEANFILES = $(desktop_DATA)
