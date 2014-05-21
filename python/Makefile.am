AUTOMAKE_OPTIONS = subdir-objects
ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS}
#SUBDIRS = tests
AM_DEFAULT_SOURCE_EXT = .cpp


#AM_CPPFLAGS = @PYTHON_CPPFLAGS@
#AM_LDFLAGS = @PYTHON_LDFLAGS@ -l@BOOST_PYTHON_LIB@


#AM_CPPFLAGS_G = $(CPPFLAGS)
#AM_LDFLAGS_G = $(LDFLAGS)
#AM_LIBADD_G = $(LIBADD)
#
#AM_CPPFLAGS = $(AM_CPPFLAGS_G)
#AM_LDFLAGS  = $(AM_LDFLAGS_G)
#AM_LIBADD   = $(AM_LIBADD_G)

LDADD = $(top_builddir)/librange/librange.la @BOOST_PYTHON_LIB@
LIBADD = $(top_builddir)/librange/librange.la @BOOST_PYTHON_LIB@

lib_LTLIBRARIES = librange_python.la

librange_python_la_CPPFLAGS = @PYTHON_CPPFLAGS@
librange_python_la_LDFLAGS = @PYTHON_LDFLAGS@ -l@BOOST_PYTHON_LIB@
librange_python_la_SOURCES = python.cpp
librange_python_la_LIBADD = $(top_builddir)/librange/librange.la


