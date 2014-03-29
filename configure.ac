AC_PREREQ([2.60])
AC_INIT([squashdelta], [0.1])
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

AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T

AC_ARG_ENABLE([lzo],
	AS_HELP_STRING([--disable-lzo], [Disable lzo support (default: autodetect)]))
AS_IF([test "x$enable_lzo" != "xno"], [
	AC_CHECK_HEADER([lzo/lzo1x.h], [
		AC_CHECK_LIB([lzo2], [lzo1x_decompress_safe], [
			AC_DEFINE([ENABLE_LZO], [1], [Define to enable LZO support])
			AC_SUBST([LZO_CFLAGS], [])
			AC_SUBST([LZO_LIBS], [-llzo2])
		])
	])
])

AC_ARG_ENABLE([lz4],
	AS_HELP_STRING([--disable-lz4], [Disable lz4 support (default: autodetect)]))
AS_IF([test "x$enable_lz4" != "xno"], [
	AC_CHECK_HEADER([lz4.h], [
		AC_CHECK_LIB([lz4], [LZ4_decompress_safe], [
			AC_DEFINE([ENABLE_LZ4], [1], [Define to enable LZ4 support])
			AC_SUBST([LZ4_CFLAGS], [])
			AC_SUBST([LZ4_LIBS], [-llz4])
		])
	])
])

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile])
AC_OUTPUT
