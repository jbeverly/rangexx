# serial 20110712001

AC_DEFUN([AX_CHECK_PROTOC], [dnl
	wanted_min_version="$1"
	wanted_min_major=regexp([$1],[^\([^\.]+\)\.?.*],[\1])
	wanted_min_minor=regexp([$1],[^[^\.]+\.\([^\.]+\)\(\.?.*\)],[\1])
	wanted_min_patch=regexp([$1],[^[^\.]+\.[^\.]+\.\(.*\)],[\1])

	AC_PATH_PROG(PROTOC, protoc)
	if test x$PROTOC = x; then
		AC_MSG_ERROR([Please install Google protobuf version ${wanted_min_version} or later and ensure protoc is in your path.])
	else
		AC_MSG_CHECKING([if protoc is > $wanted_min_version])dnl
		
		protoc_version=`$PROTOC --version | cut -d" " -f2`
		protoc_major=`echo $protoc_version | cut -d. -f1`
		protoc_minor=`echo $protoc_version | cut -d. -f2`
		protoc_major=`echo $protoc_version | cut -d. -f1`
		protoc_patch=`echo $protoc_version | cut -d. -f3`

		while true; do
			if test $protoc_major -gt $wanted_min_major; then
				break
			elif test $protoc_major -eq $wanted_min_major \
				&& test $protoc_minor -gt $wanted_min_minor; then
				break
			elif test $protoc_major -eq $wanted_min_major \
				&& test $protoc_minor -eq $wanted_min_minor \
				&& test $protoc_patch -ge $wanted_min_patch; then
				break
			fi
			AC_MSG_ERROR([Please install Google protobuf > ${wanted_min_version} and ensure protoc is in your path.])
			break
		done
		AC_MSG_RESULT([yes])
		AC_SUBST([PROTOC])
	fi
	])dnl
])
