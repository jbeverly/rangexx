#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([range],[0.1],[jamie.r.beverly@gmail.com])

AC_LANG([C++])

AC_USE_SYSTEM_EXTENSIONS
AM_PROG_AR
LT_INIT
LT_LANG([C++])
AM_INIT_AUTOMAKE([-W -Wall gnu])
AC_CONFIG_SRCDIR([range_config.h.in])
AC_CONFIG_HEADERS([range_config.h])
ACLOCAL_AMFLAGS="-I m4"
AC_CONFIG_MACRO_DIR([m4])

CPPFLAGS="$CPPFLAGS -D_FORTIFY_SOURCE=2"

PKG_PROG_PKG_CONFIG

# Checks for programs.
AC_PROG_CXX
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET

AC_CHECK_LIB([rt], [shm_open], [], AC_MSG_ERROR([*** could not find librt]))

AX_CXX_COMPILE_STDCXX_11([ext],[mandatory])
#AX_CHECK_EXT_LIB_CLASS([db_cxx], [<db_cxx.h>], [DbStream], [], [AC_MSG_ERROR([Berkeley db6 is required])])

AS_VAR_SET([tmp_stl_LDFLAGS],["$LDFLAGS"])
AS_VAR_SET([LDFLAGS], ["$LDFLAGS $DB_CXX_LDFLAGS"])
AS_VAR_SET([tmp_stl_CPPFLAGS],["$CPPFLAGS"])
AS_VAR_SET([CPPFLAGS], ["$CPPFLAGS $DB_CXX_CPPFLAGS"])
AX_CHECK_EXT_LIB_CLASS([db_stl], [<dbstl_map.h>], [dbstl::db_map<int, int>], [], [AC_MSG_ERROR([Berkeley db6 is required])])
#AS_VAR_SET([LDFLAGS], ["$LDFLAGS -ldb"])
#AX_CHECK_EXT_LIB([evpaxos], [evproposer_init], [], [AC_MSG_ERROR([libevpaxos3 is required])])
AS_VAR_SET([LDFLAGS], ["$tmp_stl_LDFLAGS"])
AS_VAR_SET([CPPFLAGS], ["$tmp_stl_CPPFLAGS"])

AX_CHECK_EXT_LIB_CLASS([protobuf], [<google/protobuf/message.h>], [google::protobuf::Message], [], [PKG_CHECK_MODULES([protobuf], [protobuf], [], [AC_MSG_ERROR([protobuf > 2.0.0 is required])])])


AX_CHECK_EXT_LIB_CLASS([gmock], [<gmock/gmock.h>], [testing::MatchResultListener], [], [AC_MSG_WARN([gmock >= 1.7.0 is required for 'make check'])])
AX_CHECK_EXT_LIB_CLASS([gtest], [<gtest/gtest.h>], [testing::Test], [], [AC_MSG_WARN([gtest >= 1.7.0 is required for 'make check'])])
AX_CHECK_PROTOC([2.0.0])

AX_BOOST_BASE([1.55],[], [AC_MSG_ERROR([Boost >= 1_55 is required])])
AS_VAR_SET([tmp_boost_LDFLAGS],["$LDFLAGS"])
AS_VAR_SET([tmp_boost_CPPFLAGS],["$CPPFLAGS"])
AS_VAR_SET([CPPFLAGS],["$BOOST_CPPFLAGS $CPPFLAGS"])
AS_VAR_SET([CPPFLAGS],["$BOOST_LDFLAGS $LDFLAGS"])
AX_BOOST_THREAD
AS_VAR_SET([LDFLAGS], ["$LDFLAGS $BOOST_THREAD_LIB"])
AX_BOOST_SYSTEM
AS_VAR_SET([LDFLAGS], ["$LDFLAGS $BOOST_SYSTEM_LIB"])
AX_BOOST_FILESYSTEM
AS_VAR_SET([LDFLAGS], ["$LDFLAGS $BOOST_FILESYSTEM_LIB"])
AX_BOOST_DATE_TIME
AS_VAR_SET([LDFLAGS], ["$LDFLAGS $BOOST_DATE_TIME_LIB"])
AX_BOOST_LOG
AS_VAR_SET([LDFLAGS], ["$LDFLAGS $BOOST_LOG_LIB"])
AX_BOOST_LOG_SETUP
AS_VAR_SET([LDFLAGS], ["$LDFLAGS $BOOST_LOG_SETUP_LIB"])
AX_PYTHON_DEVEL([>= '2.6.0'])
AX_BOOST_PYTHON
AS_VAR_SET([LDFLAGS], ["$LDFLAGS -l$BOOST_PYTHON_LIB"])
AX_BOOST_REGEX
AS_VAR_SET([LDFLAGS], ["$LDFLAGS $BOOST_REGEX_LIB"])

AS_VAR_SET([LDFLAGS], ["$tmp_boost_LDFLAGS"])
AS_VAR_SET([CPPFLAGS], ["$tmp_boost_CPPFLAGS"])

gl_COMMON
gl_PARSE_DATETIME
gl_TM_GMTOFF
gl_FUNC_GNU_STRFTIME
gl_EXTERN_INLINE


AC_CONFIG_FILES([Makefile
                 librange/Makefile
                 librange/tests/Makefile
				 stored/Makefile
				 stored/tests/Makefile
				 python/Makefile
                 ])

AC_OUTPUT
