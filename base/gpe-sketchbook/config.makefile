VERSION = pre0.1.5

DEPENDS = libgtk1.2, libgpewidget0

PACKAGE         = gpe-sketchbook
PACKAGE_DESCR   = a notebook to sketch your notes
MAINTAINER      = Luc Pionchon
MAINTAINER_MAIL = luc.pionchon@welho.com
SECTION         = base
PRIORITY        = optional

MEMBERS = \
	gpe-sketchbook       \
	selector	     \
	selector-cb	     \
	selector-gui	     \
	sketchpad	     \
	sketchpad-cb	     \
	sketchpad-gui	     \
	files		     \
	files-xpm	     \
	files-png	     

PIXMAPS = `cd pixmaps && ls *.png`

#EXTRA_SRC_DIST = cursor_eraser_data.h makefile
#EXTRA_TOP_DIST = gpe-sketchbook.menu gpe-sketchbook.png ipkg-build my-config makefile
#DIST_COMMON    = COPYING README INSTALL TODO AUTHORS ChangeLog

#-- PREFIX is overwritten to "/usr" by target ipkg:
PREFIX  = /usr/local

