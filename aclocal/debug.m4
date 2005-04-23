AC_DEFUN(AC_DEBUG_OPTIONS, [
AC_ARG_ENABLE(debug,
           [AC_HELP_STRING([--enable-debug=full yes no], [compile with debug info])],
                [debug=$enableval],[debug=no])
if test "x$debug" = "xfull"; then
    CFLAGS="${CFLAGS} -g3 -O0 -DDEBUG"
    CXXFLAGS="${CFLAGS}"
    AC_MSG_RESULT([building in full debug info])
elif test "x$debug" = "xyes"; then
    CFLAGS="${CFLAGS} -g -O1 -DDEBUG"
    CXXFLAGS="${CFLAGS}"
    AC_MSG_RESULT([building in debug mode])
elif test "x$debug" = "xno"; then
    CFLAGS="${CFLAGS} -O3 -DNDEBUG"
    CXXFLAGS="${CFLAGS} -O3 -finline-functions"
    AC_MSG_RESULT([building without debug info])
fi
AC_MSG_RESULT([CFLAGS: $CFLAGS, CXXFLAGS: $CXXFLAGS])
])


