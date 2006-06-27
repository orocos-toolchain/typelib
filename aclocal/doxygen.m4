dnl $Rev: 1498 $
dnl $Id: doxygen.m4 1498 2006-06-27 09:29:51Z sjoyeux $

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
	    DOT_PATH=`dirname $DOT`
	    AC_SUBST(DOT_PATH)
	else
	    HAS_DOT=no
	fi

	AC_PATH_PROG(PERL, perl)
	if test -n $PERL; then
	    HAS_PERL=yes
	    AC_SUBST(HAS_PERL)
	else
	    HAS_PERL=no
	fi

	HAS_DOXYGEN=yes
	AC_SUBST(HAS_DOXYGEN)
	ifelse([$4], [$4], [])
    ])
])

# vim: ts=8
