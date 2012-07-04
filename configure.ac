#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([range],[0.1],[jamie.r.beverly@gmail.com])

AC_LANG([C++])

AC_USE_SYSTEM_EXTENSIONS
AM_PROG_AR
LT_INIT
AM_INIT_AUTOMAKE([-W -Wall -Werror foreign])
AC_CONFIG_SRCDIR([range_config.h.in])
AC_CONFIG_HEADERS([range_config.h])
ACLOCAL_AMFLAGS="-I m4"
AC_CONFIG_MACRO_DIR([m4])

PKG_PROG_PKG_CONFIG

# Checks for programs.
AC_PROG_CXX
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET

AX_CXX_COMPILE_STDCXX_11([ext],[mandatory])
AX_CHECK_EXT_LIB_CLASS([db_cxx], [<db_cxx.h>], [Dbc], [], [AC_MSG_ERROR([Berkeley db4 is required])])
AX_CHECK_EXT_LIB_CLASS([protobuf], [<google/protobuf/message.h>], [google::protobuf::Message], [], [PKG_CHECK_MODULES([protobuf], [protobuf], [], [AC_MSG_ERROR([protobuf > 2.0.0 is required])])])
AX_CHECK_EXT_LIB_CLASS([gmock], [<gmock/gmock.h>], [testing::MatchResultListener], [], [AC_MSG_WARN([gmock >= 1.6.0 is required for 'make check'])])
AX_CHECK_EXT_LIB_CLASS([gtest], [<gtest/gtest.h>], [testing::Test], [], [AC_MSG_WARN([gtest >= 1.6.0 is required for 'make check'])])
AX_CHECK_PROTOC([2.0.0])

AX_BOOST_BASE([1.49],[], [AC_MSG_ERROR([Boost >= 1_49 is required])])
AX_BOOST_LOG
AX_BOOST_LOG_SETUP
AX_PYTHON_DEVEL([>= '2.5.0'])
AX_BOOST_PYTHON
AX_BOOST_REGEX
AX_BOOST_THREAD


AC_CONFIG_FILES([Makefile
                 librange/Makefile
                 librange/tests/Makefile
                 ])

AC_OUTPUT
