
AM_CFLAGS =\
	$(PLUGIN_CFLAGS)\
	-Wall\
	-O2\
	-g
				   

INCLUDES = -I../include/

bin_PROGRAMS = plugin_test

plugin_test_SOURCES = plugin_test.c
plugin_test_LDADD = $(PLUGIN_LIBS) ../src/libgpeplugin.la

libexec_PROGRAMS = plugin_foo.so

plugin_foo_so_SOURCES = plugin_foo.c
plugin_foo_so_CFLAGS = $(PLUGIN_CFLAGS) -g
plugin_foo_so_LDFLAGS = -shared -fPIC

