AUTOMAKE_OPTIONS = parallel-tests color-tests
export LD_LIBRARY_PATH:=@GMOCK_LD_LIBRARY_PATH@:@GTEST_LD_LIBRARY_PATH@:@PROTOBUF_LD_LIBRARY_PATH@:@DB_STL_LD_LIBRARY_PATH@:${LD_LIBRARY_PATH}

AM_CPPFLAGS =  @PROTOBUF_CPPFLAGS@ @DB_STL_CPPFLAGS@ @BOOST_CPPFLAGS@ @GMOCK_CPPFLAGS@ @GTEST_CPPFLAGS@ -I$(top_builddir)/librange
AM_LDFLAGS = -Wl,--copy-dt-needed-entries @GMOCK_LDFLAGS@ @GTEST_CPPFLAGS@ -lpthread -lrt
LDADD = @GTEST_LIBS@ @GMOCK_LIBS@ $(top_builddir)/librange/librange.la 
LIBADD = @GTEST_LIBS@ @GMOCK_LIBS@ $(top_builddir)/librange/librange.la

AM_DEFAULT_SOURCE_EXT = .cpp

check_PROGRAMS = test_stored


TESTS = $(check_PROGRAMS)

.PHONY: valgrind

valgrind: $(TESTS)
	for n in $(TESTS); do \
		LD_LIBRARY_PATH="${LD_LIBRARY_PATH}:$(top_builddir)/librange/.libs/" \
		valgrind -q --tool=memcheck --leak-check=full --leak-resolution=high --track-origins=yes --show-reachable=yes $(builddir)/.libs/$$n; \
	done



