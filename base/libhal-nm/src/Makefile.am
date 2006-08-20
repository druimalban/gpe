lib_LTLIBRARIES = libhal-nm.la
libhal_nm_la_SOURCES = libhal.c libhal-nm-backend.c libhal-nm-backend.h backend-net.c backend-net.h

include_HEADERS = libhal.h
libhal_nm_la_CFLAGS = $(DBUS_CFLAGS) $(GLIB_CFLAGS) -DDBUS_API_SUBJECT_TO_CHANGE -Wall
libhal_nm_la_LIBADD = $(DBUS_LIBS) $(GLIB_LIBS)
