SUBDIRS = doc

lib_LTLIBRARIES = libeventdb.la

AM_CPPFLAGS = @EVENTDB_CFLAGS@ -D_XOPEN_SOURCE -DPACKAGE_LOCALE_DIR=\"@datadir@/locale/@PACKAGE_NAME@\" -D_GNU_SOURCE

libeventdb_la_LDFLAGS = @EVENTDB_LIBS@ -version-info 1:0:0

libeventdb_la_SOURCES = event-db.c gpe/event-db.h

gpeincludedir = $(includedir)/gpe
gpeinclude_HEADERS = gpe/event-db.h

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libeventdb.pc

DISTCHECK_CONFIGURE_FLAGS = --enable-gtk-doc

# Test suite
check_PROGRAMS = test-event-list test-recur-daily test-recur-weekly	\
	test-recur-monthly test-alarms
TESTS_ENVIRONMENT = /bin/sh $(srcdir)/test.sh
TESTS = test-event-list test-recur-daily test-recur-weekly	\
	test-recur-monthly test-alarms

test_event_list_SOURCES = test-event-list.c gpe/event-db.h test-skeleton.h
test_event_list_LDADD = libeventdb.la
test_recur_daily_SOURCES = test-recur-daily.c gpe/event-db.h test-skeleton.h
test_recur_daily_LDADD = libeventdb.la
test_recur_weekly_SOURCES = test-recur-weekly.c gpe/event-db.h test-skeleton.h
test_recur_weekly_LDADD = libeventdb.la
test_recur_monthly_SOURCES = test-recur-monthly.c gpe/event-db.h	\
	test-skeleton.h
test_recur_monthly_LDADD = libeventdb.la
test_alarms_SOURCES = test-alarms.c gpe/event-db.h test-skeleton.h
test_alarms_LDADD = libeventdb.la

include gpe-dist.am
