
all: doc
doc:
	gtkdoc-scan --module=$(PACKAGE) --source-dir=../gpe --output-dir=.
	gtkdoc-mktmpl --module=$(PACKAGE)
	gtkdoc-mkdb --module=$(PACKAGE) --source-dir=../gpe --tmpl-dir=tmpl --output-format=xml --main-sgml-file=$(PACKAGE)-docs.sgml --sgml-mode
	gtkdoc-mkhtml $(PACKAGE) $(PACKAGE)-docs.sgml
clean:
	rm -f *.html sgml/* xml/* *.stamp

install-devel: all
