bin_PROGRAMS = gpe-memo
gpe_memo_SOURCES = main.c interface.c sound.c \
			gsm-codec.c settings.c
noinst_HEADERS = interface.h sound.h gsm-codec.h settings.h
gpe_memo_CFLAGS = @GTK_CFLAGS@ @GPE_CFLAGS@ @ESD_CFLAGS@
gpe_memo_LDADD = @GTK_LIBS@ @GPE_LIBS@ @ESD_LIBS@ -lgsm
