AUTOMAKE_OPTIONS = color-tests
export LD_LIBRARY_PATH:=@GMOCK_LD_LIBRARY_PATH@:@GTEST_LD_LIBRARY_PATH@:@PROTOBUF_LD_LIBRARY_PATH@:@DB_CXX_LD_LIBRARY_PATH@:${LD_LIBRARY_PATH}

AM_CPPFLAGS = @GMOCK_CPPFLAGS@ @GTEST_CPPFLAGS@ @PROTOBUF_CPPFLAGS@ @DB_CXX_CPPFLAGS@ @DB_STL_CPPFLAGS@ -I$(top_builddir)/librange
AM_LDFLAGS = @GMOCK_LDFLAGS@ @GTEST_CPPFLAGS@ @PROTOBUF_LDFLAGS@ @DB_CXX_LDFLAGS@ @DB_STL_LDFLAGS@ 
LDADD = $(top_builddir)/librange/librange.la @GTEST_LIBS@ @GMOCK_LIBS@ @DB_CXX_LIBS@ @DB_STL_LIBS@
LIBADD = $(top_builddir)/librange/librange.la

AM_DEFAULT_SOURCE_EXT = .cpp

check_PROGRAMS = test_graph_iterator \
				 test_pbuff_node \
				 test_db \
				 test_graphdb \
				 test_compiler

TESTS = $(check_PROGRAMS)

.PHONY: valgrind

valgrind: $(TESTS)
	for n in $(TESTS); do \
		LD_LIBRARY_PATH="${LD_LIBRARY_PATH}:$(top_builddir)/librange/.libs/" \
		valgrind -q --tool=memcheck --leak-check=full --leak-resolution=high --track-origins=yes --show-reachable=yes $(builddir)/.libs/$$n; \
	done



