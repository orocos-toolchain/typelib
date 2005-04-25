
AC_DEFUN(CLBS_DEBUG_RESULT, [
  AC_MSG_RESULT([$1. Flags: $CFLAGS])
])

AC_DEFUN(CLBS_DEBUG_OPTIONS, [
  AC_MSG_CHECKING([if we are building for debug])
  AC_ARG_ENABLE(debug,
             AC_HELP_STRING([--enable-debug=full yes no], [compile with debug info (default=yes)]),
             [debug=$enableval],[debug=yes])

  if test "x$debug" = "xfull"; then
      CFLAGS="${CFLAGS} -g3 -O0 -DDEBUG"
      CXXFLAGS="${CFLAGS}"
      CLBS_DEBUG_RESULT([yes, full])
  elif test "x$debug" = "xyes"; then
      CFLAGS="${CFLAGS} -g -O1 -DDEBUG"
      CXXFLAGS="${CFLAGS}"
      CLBS_DEBUG_RESULT([yes])
  elif test "x$debug" = "xno"; then
      CFLAGS="${CFLAGS} -O3 -DNDEBUG"
      CXXFLAGS="${CFLAGS} -O3 -finline-functions"
      CLBS_DEBUG_RESULT([no])
  fi
])


