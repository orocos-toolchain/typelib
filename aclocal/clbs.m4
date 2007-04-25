dnl $Rev: 1401 $
dnl $Id: clbs.m4 1401 2006-02-09 13:22:04Z sjoyeux $

AC_DEFUN([FPY_DIRNAME_EXPR],
         [[expr ".$1" : '\(\.\)[^/]*$' \| "x$1" : 'x\(.*\)/[^/]*$']])

AC_DEFUN([FPY_PATH_PROG],
  [
   AC_PATH_TOOL([$1], [$2], no, [$5:$PATH])
   if test "x$1" = "xno" ; then 
	ifelse([$4], ,:, [$4])
   else
	ifelse([$3], ,:, [$3])
   fi
])

AC_DEFUN([CLBS_GET_USRLIBDIR], [
    AC_CHECK_SIZEOF(long int)
    usrlibdir=lib
    if test "$ac_cv_sizeof_long_int" -eq 8; then
      if test -d /usr/lib64; then
        usrlibdir=lib64
      fi
    fi
    AC_MSG_RESULT(Using $usrlibdir for system libraries)
  ])

AC_DEFUN([CLBS_TEST_SUPPORT], [
     AC_REQUIRE([CLBS_BOOST_TEST])
     AC_ARG_ENABLE(test, 
	AC_HELP_STRING([--disable-test], [enable test suite (default=yes if boost/test is present)]),
	[HAS_TEST_SUPPORT=$enableval],[HAS_TEST_SUPPORT=$HAS_BOOST_TEST])

     AC_SUBST(HAS_TEST_SUPPORT)
     AS_IF([test "x$HAS_TEST_SUPPORT" = "xyes"],
        [AC_MSG_NOTICE([Enabling test support])
         CLBS_TEST_CPPFLAGS=$BOOST_TEST_CPPFLAGS
         CLBS_TEST_LDFLAGS=$BOOST_TEST_LDFLAGS
         AC_SUBST(CLBS_TEST_CPPFLAGS)
         AC_SUBST(CLBS_TEST_LDFLAGS)
         $1],
         [$2])
])

AC_DEFUN([CLBS_PCH_SUPPORT], [
AC_ARG_WITH(pch,
           AC_HELP_STRING([--with-pch=header.h], [use precompiled headers]),
                [pch=$withval],[pch=no])
if ! test "x$pch" = "xno"; then
    USE_PCH=1
    PCH_HEADER=$pch

    AC_MSG_RESULT(using $pch as precompiled header)
fi
AC_SUBST(USE_PCH)
AC_SUBST(PCH_HEADER)
])

AC_DEFUN([CLBS_DEBUG_RESULT], [
  AC_MSG_RESULT([$1. Flags: $CFLAGS])
])

AC_DEFUN([CLBS_DEBUG_OPTIONS], [
  AC_MSG_CHECKING([if we are building for debug])
  AC_ARG_ENABLE(debug,
             AC_HELP_STRING([--enable-debug=full yes no], [compile with debug info (default=yes)]),
             [debug=$enableval],[debug=yes])

  if test "x$debug" = "xfull"; then
      CFLAGS="${CFLAGS} -g3 -O0 -DDEBUG"
      CXXFLAGS="${CFLAGS}"
      CLBS_DEBUG_RESULT([yes, full])
  elif test "x$debug" = "xno"; then
      CFLAGS="${CFLAGS} -O3 -DNDEBUG"
      CXXFLAGS="${CFLAGS} -O3 -finline-functions"
      CLBS_DEBUG_RESULT([no])
  else
      CFLAGS="${CFLAGS} -g -O1 -DDEBUG"
      CXXFLAGS="${CFLAGS}"
      CLBS_DEBUG_RESULT([yes])
  fi
])


