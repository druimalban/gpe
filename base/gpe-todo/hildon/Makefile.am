desktopdir = $(datadir)/applications/hildon
desktop_in_files = gpe-todo.desktop.in

servicedir = $(datadir)/dbus-1/services
service_in_files = com.nokia.gpe-todo.service.in

desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
service_DATA = $(service_in_files:.service.in=.service)
@INTLTOOL_DESKTOP_RULE@

pixmapsdir = $(datadir)/pixmaps
# we need a 26x26 icon here 
pixmaps_DATA = gpe-todo.png

gpe-todo.png: gpe-todo-48x48.png
	cp $< $@

iconsdir = $(datadir)/icons/hicolor

#nobase_icons_DATA = 26x26/hildon/gpe-todo.png 40x40/hildon/gpe-todo.png 48x48/hildon/gpe-todo.png 64x64/hildon/gpe-todo.png scalable/hildon/gpe-todo.png
nobase_icons_DATA = 26x26/hildon/gpe-todo.png scalable/hildon/gpe-todo.png

26x26/hildon/gpe-todo.png: gpe-todo-26x26.png
	-mkdir -p 26x26/hildon
	cp $< $@

40x40/hildon/gpe-todo.png: gpe-todo-40x40.png
	-mkdir -p 40x40/hildon
	cp $< $@

48x48/hildon/gpe-todo.png: gpe-todo-48x48.png
	-mkdir -p 48x48/hildon
	cp $< $@

64x64/hildon/gpe-todo.png: gpe-todo-64x64.png
	-mkdir -p 64x64/hildon
	cp $< $@

scalable/hildon/gpe-todo.png: gpe-todo-64x64.png
	-mkdir -p scalable/hildon
	cp $< $@


EXTRA_DIST = $(service_DATA) \
	     $(desktop_DATA) \
	     $(pixmaps_DATA) \
	     $(nobase_icons_DATA)
             
