PREFIX  = /usr/local
#-- PREFIX is overwritten to "/usr" by target ipkg:
#--------------- that's bad because the pgme uses PREFIX (icon path) !!!

#VERSION = pre0.1.5

LINGUAS = fr
MEMBERS = \
	gpe-sketchbook       \
	dock                 \
	dialog               \
	note                 \
	selector	     \
	selector-cb	     \
	selector-gui	     \
	sketchpad	     \
	sketchpad-cb	     \
	sketchpad-gui	     \
	files		     \
	files-png	     

#PIXMAPS = ...

#PACKAGE         = gpe-sketchbook
#PACKAGE_DESCR   = a notebook to sketch your notes
#MAINTAINER      = Luc Pionchon
#MAINTAINER_MAIL = luc.pionchon@welho.com
#SECTION         = base
#PRIORITY        = optional
#DEPENDS         = libgtk1.2, libgpewidget0, libpng2, libgdk-pixbuf2

#EXTRA_SRC_DIST = cursor_eraser_data.h makefile
#EXTRA_TOP_DIST = gpe-sketchbook.menu gpe-sketchbook.png ipkg-build my-config makefile
#DIST_COMMON    = COPYING README INSTALL TODO AUTHORS ChangeLog

