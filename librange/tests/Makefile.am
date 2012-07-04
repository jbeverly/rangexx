AUTOMAKE_OPTIONS = color-tests
export LD_LIBRARY_PATH=@GMOCK_LD_LIBRARY_PATH@:@GTEST_LD_LIBRARY_PATH@:@PROTOBUF_LD_LIBRARY_PATH@:@DB_CXX_LD_LIBRARY_PATH@

AM_CPPFLAGS = @GMOCK_CPPFLAGS@ @GTEST_CPPFLAGS@ @PROTOBUF_CPPFLAGS@ -I$(top_builddir)/librange
AM_LDFLAGS = @GMOCK_LDFLAGS@ @GTEST_CPPFLAGS@ @PROTOBUF_LDFLAGS@
LDADD = @GTEST_LIBS@ @GMOCK_LIBS@ $(top_builddir)/librange/librange.la

check_PROGRAMS = graph_iterator
graph_iterator_SOURCES = graph_iterator.cpp 


TESTS = graph_iterator

.PHONY: valgrind
valgrind: $(TESTS)
	for n in $(TESTS); do \
		LD_LIBRARY_PATH="${LD_LIBRARY_PATH}:$(top_builddir)/librange/.libs/" \
		valgrind -q --tool=memcheck --leak-check=full --leak-resolution=high --track-origins=yes --show-reachable=yes $(builddir)/.libs/$$n; \
	done



