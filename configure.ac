AC_PREREQ([2.60])
AC_INIT([squashfs-delta-tools], [0])
AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE([1.6 foreign dist-bzip2 subdir-objects])

AC_LANG([C++])
AC_PROG_CXX

AC_USE_SYSTEM_EXTENSIONS
AC_CHECK_HEADERS([endian.h],, [
	is_le=0
	AC_C_BIGENDIAN(, [is_le=1])

	AS_IF([test $is_le = 1], [
		AC_DEFINE([le16toh(x)], [x], [fake le16toh])
		AC_DEFINE([le32toh(x)], [x], [fake le32toh])
		AC_DEFINE([le64toh(x)], [x], [fake le64toh])
	], [
		AC_MSG_ERROR([Big endian support requires endian.h.])
	])
])

AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile])
AC_OUTPUT
