AUTOMAKE_OPTIONS = subdir-objects
ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS}
SUBDIRS = tests
AM_DEFAULT_SOURCE_EXT = .cpp

AM_CPPFLAGS_G = $(CPPFLAGS)
AM_LDFLAGS_G = $(LDFLAGS)
#AM_LIBADD_G = $(LIBADD)


AM_CPPFLAGS = @PROTOBUF_CPPFLAGS@ @DB_CXX_CPPFLAGS@ @BOOST_CPPFLAGS@ @GMOCK_CPPFLAGS@ @GTEST_CPPFLAGS@ -I$(top_builddir)/librange $(AM_CPPFLAGS_G)
AM_LDFLAGS  = -L$(top_builddir)/librange -Wl,--copy-dt-needed-entries -lrange -lpthread -lrt $(AM_LDFLAGS_G)
#AM_LIBADD   = $(AM_LIBADD_G)

noinst_LTLIBRARIES = libstored.la

libstored_la_LDFLAGS = -Wl,--no-undefined
libstored_la_SOURCES = worker_thread.cpp \
					   mqserv.cpp \
					   network.cpp \
					   listenserv.cpp \
					   signalhandler.cpp \
					   paxos.cpp \
					   paxos_proposer.cpp \
					   paxos_accepter.cpp \
					   paxos_learner.cpp \
					   startup.cpp \
					   range_client.cpp \
					   heartbeat.cpp

#LDADD = $(top_builddir)/librange/librange.la
#LIBADD = $(top_builddir)/librange/librange.la

sbin_PROGRAMS = stored
stored_SOURCES = stored.cpp
stored_LDADD = libstored.la


