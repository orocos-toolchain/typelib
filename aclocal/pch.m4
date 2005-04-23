AC_DEFUN(AC_PCH_SUPPORT, [
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

