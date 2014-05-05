AUTOMAKE_OPTIONS = subdir-objects
ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS}
SUBDIRS = tests

AM_CPPFLAGS_G = $(CPPFLAGS)
AM_LDFLAGS_G = $(LDFLAGS)
AM_LIBADD_G = $(LIBADD)

AM_CPPFLAGS = $(AM_CPPFLAGS_G)
AM_LDFLAGS  = $(AM_LDFLAGS_G)
AM_LIBADD   = $(AM_LIBADD_G)

LDADD = $(top_builddir)/librange/librange.la
LIBADD = $(top_builddir)/librange/librange.la

AM_DEFAULT_SOURCE_EXT = .cpp

