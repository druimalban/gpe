if HILDON

#nobase_icons_DATA = 26x26/hildon/gpe-filemanager.png 40x40/hildon/gpe-filemanager.png 48x48/hildon/gpe-filemanager.png 64x64/hildon/gpe-filemanager.png scalable/hildon/gpe-filemanager.png
nobase_icons_DATA = 26x26/hildon/gpe-filemanager.png scalable/hildon/gpe-filemanager.png

else

nobase_icons_DATA = 

endif

gpe-filemanager.png: gpe-filemanager-48x48.png
	cp $< $@

iconsdir = $(datadir)/icons/hicolor

26x26/hildon/gpe-filemanager.png: gpe-filemanager-26x26.png
	-mkdir -p 26x26/hildon
	cp $< $@

40x40/hildon/gpe-filemanager.png: gpe-filemanager-40x40.png
	-mkdir -p 40x40/hildon
	cp $< $@

48x48/hildon/gpe-filemanager.png: gpe-filemanager-48x48.png
	-mkdir -p 48x48/hildon
	cp $< $@

64x64/hildon/gpe-filemanager.png: gpe-filemanager-64x64.png
	-mkdir -p 64x64/hildon
	cp $< $@

scalable/hildon/gpe-filemanager.png: gpe-filemanager-64x64.png
	-mkdir -p scalable/hildon
	cp $< $@

pixmapsextradir = $(datadir)/gpe/pixmaps/default/filemanager/document-icons/
pixmapsextra_DATA = \
                   document-icons/directory.png \
                   document-icons/image-png.png \
                   document-icons/executable.png \
                   document-icons/image-xpm.png \
                   document-icons/application-pdf.png \
                   document-icons/text-plain.png \
                   document-icons/application-x-compress.png \
                   document-icons/text-html.png \
                   document-icons/application-x-gzip.png \
                   document-icons/text-x-copying.png \
                   document-icons/application-x-ipk.png \
                   document-icons/text-x-credits.png \
                   document-icons/application-x-ogg.png \
                   document-icons/text-xml.png \
                   document-icons/audio-mpeg.png \
                   document-icons/text-x-python.png \
                   document-icons/audio-x-gsm.png \
                   document-icons/text-x-sql.png \
                   document-icons/audio-x-wav.png \
                   document-icons/video-mpeg.png \
                   document-icons/font-ttf.png \
                   document-icons/video-x-msvideo.png \
                   document-icons/image-gif.png \
                   document-icons/regular.png \
                   document-icons/image-jpeg.png \
                   document-icons/symlink.png


pixmapsdir = $(datadir)/pixmaps
pixmaps_DATA = gpe-filemanager.png

EXTRA_DIST =  \
             $(pixmaps_DATA) \
             $(pixmapsextra_DATA)
