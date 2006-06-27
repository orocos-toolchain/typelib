dnl $Rev: 1495 $
dnl $Id: doxygen.m4 1495 2006-06-27 07:54:04Z sjoyeux $

AC_DEFUN([CLBS_CHECK_DOXYGEN], 
[
    AC_PATH_PROG(DOXYGEN, doxygen)
    AS_IF([test -z "$DOXYGEN"], [
	HAS_DOXYGEN=no
	AC_SUBST(HAS_DOXYGEN)
	ifelse([$4], [$4], [])
    ], [
	AC_PATH_PROG(DOT, dot)
	if test -n $DOT; then
	    HAS_DOT=yes
	    AC_SUBST(HAS_DOT)
	fi

	HAS_DOXYGEN=yes
	AC_SUBST(HAS_DOXYGEN)
	ifelse([$4], [$4], [])
    ])
])

# vim: ts=8
