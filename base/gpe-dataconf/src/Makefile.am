## Process this file with automake to produce Makefile.in

## Created by Anjuta - will be overwritten
## If you don't want it to overwrite it,
## 	Please disable it in the Anjuta project configuration

INCLUDES = \
	$(GTK_CFLAGS)

CFLAGS =\
	 -Wall -Wimplicit -Wreturn-type -Wunused -Wswitch -Wcomment -Wuninitialized -Wparentheses -Wtraditional -Wpointer-arith -Wmissing-prototypes\
	 -O1\
	 -g

bin_PROGRAMS = gpe_dataconf

gpe_dataconf_SOURCES = \
	main.c\
	callbacks.c\
	interface.c\
	support.c\
	db.c\
	questionfx.c\
	usqld-cfgdb.c

gpe_dataconf_LDFLAGS = 

gpe_dataconf_LDADD =  \
	$(GTK_LIBS)

