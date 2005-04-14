# The name of the module.
DOC_MODULE=libgpewidget

# The top-level SGML file.
DOC_MAIN_SGML_FILE=libgpewidget-docs.sgml

EXTRA_DIST = style.css $(DOC_MAIN_SGML_FILE) index.sgml \
             tmpl/dirbrowser.sgml    tmpl/gpeiconlistitem.sgml \
 			 tmpl/gpewindowlist.sgml  tmpl/init.sgml  tmpl/popup_menu.sgml \
			 tmpl/spacing.sgml     tmpl/windows.sgml tmpl/errorbox.sgml \
			 tmpl/gpe-iconlist.sgml     tmpl/gtkdatecombo.sgml \
 			 tmpl/libgpewidget-unused.sgml  tmpl/popup.sgml tmpl/stylus.sgml \
		     tmpl/gpeclockface.sgml  tmpl/gpeiconlistview.sgml  \
			 tmpl/gtksimplemenu.sgml  tmpl/picturebutton.sgml \
			 tmpl/question.sgml tmpl/translabel.sgml tmpl/gpehelp.sgml \
			 tmpl/gpetimesel.sgml  tmpl/infoprint.sgml tmpl/pixmaps.sgml \
			 tmpl/smallbox.sgml    tmpl/tray.sgml
			 
MKDB_OPTIONS=--sgml-mode --output-format=xml --tmpl-dir=tmpl

if ENABLE_GTK_DOC
all: doc
doc:
	gtkdoc-scan --module=$(DOC_MODULE) --source-dir=../gpe --source-dir=.. --output-dir=.
	gtkdoc-mktmpl --module=$(DOC_MODULE)
	gtkdoc-mkdb --module=$(DOC_MODULE) $(MKDB_OPTIONS) --source-dir=../gpe --source-dir=.. --main-sgml-file=$(DOC_MAIN_SGML_FILE)
	gtkdoc-mkhtml $(DOC_MODULE) $(DOC_MAIN_SGML_FILE)
endif

clean-local:
	rm -f *.html sgml/* xml/* *.stamp

install-devel: all
