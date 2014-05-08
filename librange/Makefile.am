AUTOMAKE_OPTIONS = subdir-objects
ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS}
SUBDIRS = tests

RANGEXX_LIBRARY_VERSION = 0:0:0

AM_CPPFLAGS_G = $(CPPFLAGS)
AM_LDFLAGS_G = $(LDFLAGS)
AM_LIBADD_G = $(LIBADD)

AM_CPPFLAGS = @BOOST_CPPFLAGS@ @DB_CXX_CPPFLAGS@ @DB_STL_CPPFLAGS@ @PROTOBUF_CPPFLAGS@ $(AM_CPPFLAGS_G)
AM_LDFLAGS  = @BOOST_LDFLAGS@ @DB_CXX_LDFLAGS@  @DB_STL_LDFLAGS@ @PROTOBUF_LDFLAGS@  $(AM_LDFLAGS_G)
AM_LIBADD   = @BOOST_REGEX_LIB@ @DB_CXX_LIBS@     @DB_STL_LIBS@ @PROTOBUF_LIBS@     $(AM_LIBADD_G)

lib_LTLIBRARIES    = librange.la
noinst_LTLIBRARIES = librange_db.la librange_graph.la librange_compiler.la librange_util.la librange_core.la

core/store.pb.h: core/store.proto
	    $(PROTOC) -I=core/ --cpp_out=core/ core/store.proto && mv core/store.pb.cc core/store.pb.cpp

core/store.pb.cpp: core/store.pb.h

db/nodeinfo.pb.h: db/nodeinfo.proto
	$(PROTOC) -I=db/ --cpp_out=db/ db/nodeinfo.proto && mv db/nodeinfo.pb.cc db/nodeinfo.pb.cpp

db/nodeinfo.pb.cpp: db/nodeinfo.pb.h

db/graph_list.pb.h: db/graph_list.proto
	$(PROTOC) -I=db/ --cpp_out=db/ db/graph_list.proto && mv db/graph_list.pb.cc db/graph_list.pb.cpp

db/graph_list.pb.cpp: db/graph_list.pb.h

db/changelist.pb.h: db/changelist.proto
	$(PROTOC) -I=db/ --cpp_out=db/ db/changelist.proto && mv db/changelist.pb.cc db/changelist.pb.cpp

db/changelist.pb.cpp: db/changelist.pb.h

BUILT_SOURCES = db/nodeinfo.pb.h \
				db/nodeinfo.pb.cpp \
				db/graph_list.pb.h \
				db/graph_list.pb.cpp \
				core/store.pb.cpp

librange_util_la_includedir = $(includedir)
nobase_librange_util_la_include_HEADERS = util/crc32.h
librange_util_la_SOURCES = util/crc32.cpp
librange_util_la_LIBADD = $(AM_LIBADD)

librange_compiler_la_includedir = $(includedir)
nobase_librange_compiler_la_include_HEADERS = compiler/ast.h \
											  compiler/compiler_types.h \
											  compiler/grammar_factory.h \
											  compiler/grammar_interface.h \
											  compiler/RangeParser_v1base.h \
											  compiler/RangeParser_v1.h \
											  compiler/RangeScanner_v1base.h \
											  compiler/RangeScanner_v1.h \
											  compiler/scanner_input.h.in \
											  compiler/scanner_input.cpp.in \
											  compiler/expanding_visitor.h 
											
										
librange_compiler_la_SOURCES = compiler/RangeParser_v1.cpp \
							   compiler/RangeScanner_v1.cpp \
							   compiler/expanding_visitor.cpp
librange_compiler_la_LIBADD = $(AM_LIBADD)


librange_db_la_includedir = $(includedir)
nobase_librange_db_la_include_HEADERS  = db/graphdb.h \
										 db/pbuff_node.h \
										 db/db_interface.h \
										 db/node_factory.h \
										 db/db_exceptions.h \
										 db/berkeley_db.h \
										 db/berkeley_db_graph.h \
										 db/berkeley_db_cursor.h \
										 db/berkeley_db_weak_deleter.h \
										 db/berkeley_db_txn.h \
										 db/berkeley_db_lock.h \
										 db/berkeley_db_types.h \
										 db/config_interface.h \
										 db/changelist.pb.h \
										 db/nodeinfo.pb.h \
										 db/graph_list.pb.h

librange_db_la_SOURCES = db/graphdb.cpp \
						 db/pbuff_node.cpp \
						 db/nodeinfo.pb.cpp \
						 db/berkeley_db_cursor.cpp \
						 db/berkeley_db_graph.cpp \
						 db/berkeley_db_lock.cpp \
						 db/berkeley_db_txn.cpp \
						 db/berkeley_db.cpp \
						 db/graph_list.pb.cpp \
						 db/changelist.pb.cpp \
						 $(top_builddir)/range_config.h 

librange_db_la_LIBADD = $(AM_LIBADD)

librange_graph_la_includedir = $(includedir)
nobase_librange_graph_la_include_HEADERS = graph/graph_interface.h graph/node_interface.h
librange_graph_la_SOURCES = graph/graph_iter.cpp
librange_graph_la_LIBADD = $(AM_LIBADD)


librange_core_la_includedir = $(includedir)
nobase_librange_core_la_include_HEADERS = core/exceptions.h core/config_interface.h
librange_core_la_SOURCES = ./core/store.pb.cpp \
						   ./core/stored_message.cpp 
	
librange_core_la_LIBADD = $(AM_LIBADD)


librange_la_SOURCES = 
librange_la_LDFLAGS = -version-info $(RANGEXX_LIBRARY_VERSION)
librange_la_LIBADD = librange_graph.la librange_db.la librange_compiler.la librange_util.la 

#nobase_librange_la_include_HEADERS = 

valgrind:
	(cd tests; make valgrind)
