LINGUAS = de

.SUFFIXES: .mo .po

mo-files = $(patsubst %,%.mo,$(LINGUAS))

all: $(mo-files)

install: all
	for i in $(LINGUAS); do install -D $$i.mo $(DESTDIR)$(PREFIX)/share/locale/$$i/LC_MESSAGES/gpe-login.mo; done

.po.mo:;
	msgfmt -o $@ $<
