# The name of the module.
DOC_MODULE=libgpepimc

# The top-level SGML file.
DOC_MAIN_SGML_FILE=libgpepimc-docs.sgml

EXTRA_DIST = style.css $(DOC_MAIN_SGML_FILE) index.sgml \
             tmpl/pim-categories-ui.sgml tmpl/pim-categories.sgml
              
MKDB_OPTIONS=--sgml-mode --output-format=xml --tmpl-dir=tmpl

if ENABLE_GTK_DOC
all: doc
doc:
	gtkdoc-scan --module=$(DOC_MODULE) --source-dir=../gpe --source-dir=.. --output-dir=.
	gtkdoc-mktmpl --module=$(DOC_MODULE)
	gtkdoc-mkdb --module=$(DOC_MODULE) $(MKDB_OPTIONS) --source-dir=../gpe --source-dir=.. --main-sgml-file=$(DOC_MAIN_SGML_FILE)
	gtkdoc-mkhtml $(DOC_MODULE) $(DOC_MAIN_SGML_FILE)
endif

clean:
	rm -f *.html sgml/* xml/* *.stamp

install-devel: all
