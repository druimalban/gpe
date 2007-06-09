SUBDIRS = doc

lib_LTLIBRARIES = libeventdb.la

INCLUDES = @EVENTDB_CFLAGS@ -D_XOPEN_SOURCE -DPACKAGE_LOCALE_DIR=\"@datadir@/locale/@PACKAGE_NAME@\" -D_GNU_SOURCE

libeventdb_la_LDFLAGS = @EVENTDB_LIBS@ -version-info 2:0:0

libeventdb_la_SOURCES = gpe/event-db.h		\
              event-cal.c			\
              event-cal.h			\
              event-db.c			\
              event-db.h			\
              event.c				\
              event.h				\
	      sqlite.c

gpeincludedir = $(includedir)/gpe
gpeinclude_HEADERS = gpe/event-db.h

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libeventdb.pc

DISTCHECK_CONFIGURE_FLAGS = --enable-gtk-doc

# Test suite
check_PROGRAMS = test-event-list test-recur-daily test-recur-weekly	\
	test-recur-weekly-2 test-recur-monthly test-alarms test-eventid \
	test-find-by-uid
TESTS_ENVIRONMENT = $(SHELL) $(top_srcdir)/test.sh
TESTS = test-event-list test-recur-daily test-recur-weekly	\
	test-recur-weekly-2 test-recur-monthly test-alarms test-eventid \
	test-find-by-uid

TESTS_EXPECTED = test-event-list.expected test-recur-daily.expected \
                 test-recur-weekly.expected test-recur-weekly-2.expected \
                 test-recur-monthly.expected test-alarms.expected \
                 test-eventid.expected test-find-by-uid.expected

test_event_list_SOURCES = test-event-list.c gpe/event-db.h test-skeleton.h
test_event_list_LDADD = libeventdb.la
test_recur_daily_SOURCES = test-recur-daily.c gpe/event-db.h test-skeleton.h
test_recur_daily_LDADD = libeventdb.la
test_recur_weekly_SOURCES = test-recur-weekly.c gpe/event-db.h test-skeleton.h
test_recur_weekly_LDADD = libeventdb.la
test_recur_weekly_2_SOURCES = test-recur-weekly-2.c gpe/event-db.h \
	test-skeleton.h
test_recur_weekly_2_LDADD = libeventdb.la
test_recur_monthly_SOURCES = test-recur-monthly.c gpe/event-db.h	\
	test-skeleton.h
test_recur_monthly_LDADD = libeventdb.la
test_alarms_SOURCES = test-alarms.c gpe/event-db.h test-skeleton.h
test_alarms_LDADD = libeventdb.la
test_eventid_SOURCES = test-eventid.c gpe/event-db.h test-skeleton.h
test_eventid_LDADD = libeventdb.la
test_find_by_uid_SOURCES = test-find-by-uid.c gpe/event-db.h test-skeleton.h
test_find_by_uid_LDADD = libeventdb.la



EXTRA_DIST = test.sh \
             $(TESTS_EXPECTED)

distclean-local:
	$(RM) -f *.output

include gpe-dist.am
