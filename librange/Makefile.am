AUTOMAKE_OPTIONS = subdir-objects
ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS}
SUBDIRS = tests

RANGEXX_LIBRARY_VERSION = 0:0:0

AM_CPPFLAGS_G = $(CPPFLAGS)
AM_LDFLAGS_G = $(LDFLAGS)
AM_LIBADD_G = $(LIBADD)

AM_CPPFLAGS = @PYTHON_CPPFLAGS@ @BOOST_CPPFLAGS@ @DB_CXX_CPPFLAGS@ @PROTOBUF_CPPFLAGS@ $(AM_CPPFLAGS_G)
AM_LDFLAGS  = @PYTHON_LDFLAGS@ @BOOST_LDFLAGS@ @DB_CXX_LDFLAGS@ @PROTOBUF_LDFLAGS@  $(AM_LDFLAGS_G) 
AM_LIBADD   = @BOOST_PYTHON_LIB@ @BOOST_FILESYSTEM_LIB@ @BOOST_SYSTEM_LIB@ @BOOST_LOG_LIB@ @BOOST_LOG_SETUP_LIB@ @BOOST_DATE_TIME_LIB@ @BOOST_THREAD_LIB@ @BOOST_REGEX_LIB@ @DB_CXX_LIBS@ @PROTOBUF_LIBS@ $(AM_LIBADD_G) 

lib_LTLIBRARIES    = librange.la
noinst_LTLIBRARIES = librange_db.la librange_graph.la librange_compiler.la librange_util.la librange_core.la librange_gnu.la

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
################################################################################

noinst_librange_gnu_la_includedir = $(includedir)/rangexx
noinst_librange_gnu_la_include_HEADERS = ./gnu/parse-datetime.h \
										 ./gnu/intprops.h \
										 ./gnu/verify.h \
										 ./gnu/timespec.h \
										 ./gnu/c-ctype.h \
										 ./gnu/xalloc.h \
										 ./gnu/xalloc-oversized.h \
										 ./gnu/exitfail.h \
										 ./gnu/gettext.h

librange_gnu_la_SOURCES =  	./gnu/timespec.c \
							./gnu/parse-datetime.c \
							./gnu/c-ctype.c \
							./gnu/xalloc-die.c \
							./gnu/exitfail.c \
							./gnu/xmalloc.c \
							./gnu/gettime.c



################################################################################

librange_util_la_includedir = $(includedir)/rangexx
nobase_librange_util_la_include_HEADERS = util/crc32.h \
										  ./util/demangle.h
librange_util_la_SOURCES = util/crc32.cpp \
						   ./util/demangle.cpp \
						   ./util/fdraii.cpp

#librange_util_la_LIBADD = # $(AM_LIBADD)

################################################################################

librange_compiler_la_includedir = $(includedir)/rangexx
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
#librange_compiler_la_LIBADD = # $(AM_LIBADD)

################################################################################

librange_db_la_includedir = $(includedir)/rangexx
nobase_librange_db_la_include_HEADERS  = ./db/berkeley_dbcxx_backend.h \
										 ./db/berkeley_dbcxx_cursor.h \
										 ./db/berkeley_dbcxx_db.h \
										 ./db/berkeley_dbcxx_env.h \
										 ./db/berkeley_dbcxx_lock.h \
										 ./db/berkeley_dbcxx_txn.h \
										 ./db/berkeley_db_types.h \
										 ./db/changelist.pb.h \
										 ./db/config_interface.h \
										 ./db/db_exceptions.h \
										 ./db/db_interface.h \
										 ./db/graph_list.pb.h \
										 ./db/nodeinfo.pb.h \
										 ./db/pbuff_node.h

librange_db_la_SOURCES = db/berkeley_dbcxx_backend.cpp \
						 db/berkeley_dbcxx_cursor.cpp \
						 db/berkeley_dbcxx_db.cpp \
						 db/berkeley_dbcxx_env.cpp \
						 db/berkeley_dbcxx_lock.cpp \
						 db/berkeley_dbcxx_txlog.cpp \
						 db/berkeley_dbcxx_txn.cpp \
						 db/changelist.pb.cpp \
						 db/graph_list.pb.cpp \
						 db/nodeinfo.pb.cpp \
						 db/pbuff_node.cpp \
						 db/txlog_iterator.cpp \
						 $(top_builddir)/range_config.h 

#librange_db_la_LIBADD = # $(AM_LIBADD)

################################################################################

librange_graph_la_includedir = $(includedir)/rangexx
nobase_librange_graph_la_include_HEADERS = graph/graph_exceptions.h \
										   graph/graphdb_factory.h \
										   graph/graphdb.h \
										   graph/graph_interface.h \
										   graph/node_factory.h \
										   graph/node_interface.h

librange_graph_la_SOURCES = graph/graphdb.cpp \
							graph/graph_iter.cpp \
							graph/node_interface.cpp

#librange_graph_la_LIBADD = # $(AM_LIBADD)

################################################################################

librange_core_la_includedir = $(includedir)/rangexx
nobase_librange_core_la_include_HEADERS = ./core/api.h \
										  ./core/config_builder.h \
										  ./core/config.h \
										  ./core/exceptions.h \
										  ./core/json_visitor.h \
										  ./core/mq.h \
										  ./core/range_struct.h \
										  ./core/stored_config.h \
										  ./core/stored_message.h \
										  ./core/store.pb.h \
										  ./core/log.h \
										  ./core/builtins.h \
										  ./core/range_function.h 


librange_core_la_SOURCES = ./core/config_builder.cpp \
						   ./core/json_visitor.cpp \
						   ./core/read_api.cpp \
						   ./core/stored_message.cpp \
						   ./core/store.pb.cpp \
						   ./core/write_api.cpp \
						   ./core/log.cpp \
						   ./core/builtins.cpp \
						   ./core/exceptions.cpp
	
#librange_core_la_LIBADD = # $(AM_LIBADD)


librange_la_SOURCES = 
librange_la_CXXFLAGS = -Wl,--no-as-needed
librange_la_LIBTOOLFLAGS = --tag=CXX
librange_la_LDFLAGS = -Wl,--no-allow-shlib-undefined -version-info $(RANGEXX_LIBRARY_VERSION) $(AM_LDFLAGS) -lpthread -lrt -lz -ldl -lutil
librange_la_LIBADD = librange_graph.la librange_db.la librange_compiler.la librange_util.la librange_core.la librange_gnu.la $(AM_LIBADD)

#nobase_librange_la_include_HEADERS = 

valgrind:
	(cd tests; make valgrind)
