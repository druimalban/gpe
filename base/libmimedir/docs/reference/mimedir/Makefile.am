## Process this file with automake to produce Makefile.in

# $Id: Makefile.am,v 1.6 2002/07/31 12:51:29 srittau Exp $

# The name of the module, e.g. 'glib'.
DOC_MODULE=mimedir

# The top-level SGML file. Change it if you want.
DOC_MAIN_SGML_FILE=$(DOC_MODULE)-docs.sgml

# The directory containing the source code. Relative to $(srcdir).
# gtk-doc will search all .c & .h files beneath here for inline comments
# documenting functions and macros.
DOC_SOURCE_DIR=../../..

# Extra options to supply to gtkdoc-scan.
SCAN_OPTIONS=

# Extra options to supply to gtkdoc-mkdb.
MKDB_OPTIONS=

# Extra options to supply to gtkdoc-fixref.
FIXXREF_OPTIONS=

# Used for dependencies.
HFILE_GLOB=
CFILE_GLOB=

# Header files to ignore when scanning.
IGNORE_HFILES=						\
	config.h					\
	mimedir-marshal.h

# Images to copy into HTML directory.
HTML_IMAGES =

# Extra SGML files that are included by $(DOC_MAIN_SGML_FILE).
content_files =						\
	mimedir-bib.sgml				\
	mimedir-intro.sgml

# Other files to distribute.
extra_files =

# CFLAGS and LDFLAGS for compiling scan program. Only needed if your app/lib
# contains GtkObjects/GObjects and you want to document signals and properties.
GTKDOC_CFLAGS =
GTKDOC_LIBS =

GTKDOC_CC=$(LIBTOOL) --mode=compile $(CC)
GTKDOC_LD=$(LIBTOOL) --mode=link $(CC)

# If you need to override some of the declarations, place them in this file
# and uncomment this line.
#DOC_OVERRIDES = $(DOC_MODULE)-overrides.txt
DOC_OVERRIDES=

####################################
# Everything below here is generic #
####################################

# We set GPATH here; this gives us semantics for GNU make
# which are more like other make's VPATH, when it comes to
# whether a source that is a target of one rule is then
# searched for in VPATH/GPATH.
#
GPATH = $(srcdir)

TARGET_DIR=$(HTML_DIR)/$(DOC_MODULE)

EXTRA_DIST = 				\
	$(content_files)		\
	$(HTML_IMAGES)			\
	$(DOC_MAIN_SGML_FILE)		\
	$(DOC_MODULE)-sections.txt	\
	$(DOC_MODULE)-overrides.txt

DOC_STAMPS=scan-build.stamp tmpl-build.stamp sgml-build.stamp html-build.stamp \
	   $(srcdir)/tmpl.stamp $(srcdir)/sgml.stamp $(srcdir)/html.stamp

SCANOBJ_FILES = 		 \
	$(DOC_MODULE).args 	 \
	$(DOC_MODULE).hierarchy  \
	$(DOC_MODULE).interfaces \
	$(DOC_MODULE).prerequisites \
	$(DOC_MODULE).signals

CLEANFILES = $(SCANOBJ_FILES) $(DOC_MODULE)-unused.txt $(DOC_STAMPS)

if ENABLE_GTK_DOC
all-local: html-build.stamp

#### scan ####

scan-build.stamp: $(HFILE_GLOB) $(CFILE_GLOB)
	@echo 'gtk-doc: Scanning header files'
	@-chmod -R u+w $(srcdir)
	if grep -l '^..*$$' $(srcdir)/$(DOC_MODULE).types > /dev/null 2>&1 ; then \
	    CC="$(GTKDOC_CC)" LD="$(GTKDOC_LD)" CFLAGS="$(GTKDOC_CFLAGS)" LDFLAGS="$(GTKDOC_LIBS)" gtkdoc-scangobj $(SCANGOBJ_OPTIONS) --module=$(DOC_MODULE) --output-dir=$(srcdir) ; \
	else \
	    cd $(srcdir) ; \
	    for i in $(SCANOBJ_FILES) ; do \
               test -f $$i || touch $$i ; \
	    done \
	fi
	cd $(srcdir) && \
	  gtkdoc-scan --module=$(DOC_MODULE) --source-dir=$(DOC_SOURCE_DIR) --ignore-headers="$(IGNORE_HFILES)" $(SCAN_OPTIONS) $(EXTRA_HFILES)
	touch scan-build.stamp

$(DOC_MODULE)-decl.txt $(SCANOBJ_FILES): scan-build.stamp
	@true

#### templates ####

tmpl-build.stamp: $(DOC_MODULE)-decl.txt $(SCANOBJ_FILES) $(DOC_MODULE)-sections.txt $(DOC_MODULE)-overrides.txt
	@echo 'gtk-doc: Rebuilding template files'
	@-chmod -R u+w $(srcdir)
	cd $(srcdir) && gtkdoc-mktmpl --module=$(DOC_MODULE) $(MKTMPL_OPTIONS)
	touch tmpl-build.stamp

tmpl.stamp: tmpl-build.stamp
	@true

#### xml ####

sgml-build.stamp: tmpl.stamp $(CFILE_GLOB) $(srcdir)/tmpl/*.sgml $(expand_content_files)
	@echo 'gtk-doc: Building XML'
	@-chmod -R u+w $(srcdir)
	cd $(srcdir) && \
	gtkdoc-mkdb --module=$(DOC_MODULE) --source-dir=$(DOC_SOURCE_DIR) --output-format=xml --expand-content-files="$(expand_content_files)" --main-sgml-file=$(DOC_MAIN_SGML_FILE) $(MKDB_OPTIONS)
	touch sgml-build.stamp

sgml.stamp: sgml-build.stamp
	@true

#### html ####

html-build.stamp: sgml.stamp $(DOC_MAIN_SGML_FILE) $(content_files)
	@echo 'gtk-doc: Building HTML'
	@-chmod -R u+w $(srcdir)
	rm -rf $(srcdir)/html 
	mkdir $(srcdir)/html
	cd $(srcdir)/html && gtkdoc-mkhtml $(DOC_MODULE) ../$(DOC_MAIN_SGML_FILE)
	test "x$(HTML_IMAGES)" = "x" || ( cd $(srcdir) && cp $(HTML_IMAGES) html )
	@echo 'gtk-doc: Fixing cross-references' 
	cd $(srcdir) && gtkdoc-fixxref --module-dir=html --html-dir=$(HTML_DIR) $(FIXXREF_OPTIONS)
	touch html-build.stamp
else
all-local:
endif

##############

clean-local:
	rm -f *~ *.bak
	rm -rf .libs

maintainer-clean-local: clean
	cd $(srcdir) && rm -rf xml html $(DOC_MODULE)-decl-list.txt $(DOC_MODULE)-decl.txt

install-data-local:
	installfiles=`echo $(srcdir)/html/*`; \
	if test "$$installfiles" = '$(srcdir)/html/*'; \
	then echo '-- Nothing to install' ; \
	else \
	  $(mkinstalldirs) $(DESTDIR)$(TARGET_DIR); \
	  for i in $$installfiles; do \
	    echo '-- Installing '$$i ; \
	    $(INSTALL_DATA) $$i $(DESTDIR)$(TARGET_DIR); \
	  done; \
	  echo '-- Installing $(srcdir)/html/index.sgml' ; \
	  $(INSTALL_DATA) $(srcdir)/html/index.sgml $(DESTDIR)$(TARGET_DIR) || :; \
	fi

uninstall-local:
	rm -f $(DESTDIR)$(TARGET_DIR)/*

#
# Require gtk-doc when making dist
#
if ENABLE_GTK_DOC
dist-check-gtkdoc:
else
dist-check-gtkdoc:
	@echo "*** gtk-doc should be installed and enabled in order to make dist"
endif

dist-hook: dist-check-gtkdoc dist-hook-local
	mkdir $(distdir)/tmpl
	mkdir $(distdir)/xml
	mkdir $(distdir)/html
	-cp $(srcdir)/tmpl/*.sgml $(distdir)/tmpl
	-cp $(srcdir)/xml/*.xml $(distdir)/xml
	-cp $(srcdir)/html/* $(distdir)/html
	if test -f $(srcdir)/$(DOC_MODULE).types; then \
	  cp $(srcdir)/$(DOC_MODULE).types $(distdir)/$(DOC_MODULE).types; \
	fi

.PHONY : dist-hook-local

