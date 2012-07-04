AUTOMAKE_OPTIONS = subdir-objects
ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS}
SUBDIRS = tests

RANGEXX_LIBRARY_VERSION = 0:0:0

AM_CPPFLAGS_G = $(CPPFLAGS)
AM_LDFLAGS_G = $(LDFLAGS)
AM_LIBADD_G = $(LIBADD)

AM_CPPFLAGS = @DB_CXX_CPPFLAGS@ @PROTOBUF_CPPFLAGS@ $(AM_CPPFLAGS_G)
AM_LDFLAGS  = @DB_CXX_LDFLAGS@  @PROTOBUF_LDFLAGS@  $(AM_LDFLAGS_G)
AM_LIBADD   = @DB_CXX_LIBS@     @PROTOBUF_LIBS@     $(AM_LIBADD_G)

lib_LTLIBRARIES    = librange.la
noinst_LTLIBRARIES = librange_db.la librange_graph.la # librange_util.la librange_core.la

db/adjacency_list.pb.cpp: db/adjacency_list.proto
	$(PROTOC) -I=db/ --cpp_out=db/ db/adjacency_list.proto && mv db/adjacency_list.pb.cc db/adjacency_list.pb.cpp

db/adjacency_list.pb.h: db/adjacency_list.pb.cpp

BUILT_SOURCES = db/adjacency_list.pb.h db/adjacency_list.pb.cpp

librange_db_la_includedir = $(includedir)
nobase_librange_db_la_include_HEADERS = db/adjacency_list.pb.h db/pbuff_node.h db/graphdb.h db/db_interface.h
librange_db_la_SOURCES = db/graphdb.cpp db/adjacency_list.pb.cpp  db/pbuff_node.cpp $(top_builddir)/range_config.h 
librange_db_la_LIBADD = $(AM_LIBADD)

librange_graph_la_includedir = $(includedir)
nobase_librange_graph_la_include_HEADERS = graph/graph_interface.h graph/node_interface.h
librange_graph_la_SOURCES = graph/graph_iter.cpp
librange_graph_la_LIBADD = $(AM_LIBADD)

librange_la_SOURCES = 
librange_la_LDFLAGS = -version-info $(RANGEXX_LIBRARY_VERSION)
librange_la_LIBADD = librange_db.la librange_graph.la # librange_util.la librange_core.la 

#nobase_librange_la_include_HEADERS = 

valgrind:
	(cd tests; make valgrind)
