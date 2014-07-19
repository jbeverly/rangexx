AUTOMAKE_OPTIONS = parallel-tests color-tests 
export LD_LIBRARY_PATH:=@GMOCK_LD_LIBRARY_PATH@:@GTEST_LD_LIBRARY_PATH@:@PROTOBUF_LD_LIBRARY_PATH@:@DB_CXX_LD_LIBRARY_PATH@:${LD_LIBRARY_PATH}

AM_CPPFLAGS =  @PROTOBUF_CPPFLAGS@ @DB_CXX_CPPFLAGS@ @BOOST_CPPFLAGS@ @GMOCK_CPPFLAGS@ @GTEST_CPPFLAGS@ -I$(top_builddir)/librange
AM_LDFLAGS = -Wl,--copy-dt-needed-entries @GMOCK_LDFLAGS@ @GTEST_CPPFLAGS@ -lpthread -lrt
LDADD = @GTEST_LIBS@ @GMOCK_LIBS@ $(top_builddir)/librange/librange.la ## @DB_CXX_LIBS@ @BOOST_REGEX_LIB@
LIBADD = @GTEST_LIBS@ @GMOCK_LIBS@ $(top_builddir)/librange/librange.la

AM_DEFAULT_SOURCE_EXT = .cpp

check_PROGRAMS = test_integration \
				 test_db \
				 test_graph_iterator \
				 test_pbuff_node \
				 test_graphdb \
				 test_compiler_range_scanner_v1 \
				 test_compiler_range_parser_v1 \
				 test_compiler_expanding_visitor \
				 test_mq \
				 test_write_api \
				 test_read_api


TESTS = $(check_PROGRAMS) 


.PHONY: valgrind

valgrind: $(check_PROGRAMS)
	for n in $(TESTS); do \
		LD_LIBRARY_PATH="${LD_LIBRARY_PATH}:$(top_builddir)/librange/.libs/" \
		../../libtool --mode=execute valgrind -q --tool=memcheck --leak-check=full --leak-resolution=high --track-origins=yes --show-reachable=yes --suppressions=$(top_builddir)/librange/tests/valrind_suppressions.txt $(builddir)/$$n; \
	done



