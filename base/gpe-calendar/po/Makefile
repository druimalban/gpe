LINGUAS = de pt_PT

.SUFFIXES: .mo .po

mo-files = $(patsubst %,%.mo,$(LINGUAS))

all: $(mo-files)

install: all
	for i in $(LINGUAS); do install -D $$i.mo $(DESTDIR)$(PREFIX)/share/locale/$$i/LC_MESSAGES/gpe-calendar.mo; done

.po.mo:;
	msgfmt -o $@ $<
