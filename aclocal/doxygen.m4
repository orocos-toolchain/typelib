dnl $Rev: 1039 $
dnl $Id: doxygen.m4 1039 2005-10-12 09:17:32Z sjoyeux $

dnl -*- autoconf -*-
dnl $Id: doxygen.m4 1039 2005-10-12 09:17:32Z sjoyeux $

AC_DEFUN([CLBS_INIT_DOXYGEN], 
  [
    # files
    AC_SUBST([DX_CONFIG], [ifelse([$1], [], Doxyfile, [$1])]) 
    AC_SUBST([DX_DOCDIR], [ifelse([$2], [], doc, [$2])]) 
 
    DX_PATH="" 

    AC_ARG_WITH(doxygen,
	AC_HELP_STRING([--with-doxygen=PATH]
	               [absolute path where to find doxygen]),
	[DX_PATH="$withval"])
    FPY_PATH_PROG(DX_CMD, doxygen, 
	[
	   FPY_PATH_PROG(DX_PERL, perl, 
	     [
	       AC_SUBST([DX_CMD])
	       AC_SUBST([DX_PERL])
	       FPY_PATH_PROG(DX_DOT, dot, [DX_HAVE_DOT=YES], [DX_HAVE_DOT=NO])
    	       AC_SUBST([DX_HAVE_DOT])
               AC_SUBST([DX_DOT], [`FPY_DIRNAME_EXPR($DX_DOT)`])
	     ], [$3])
	] ,[$3], [$DX_PATH])

  ])

# vim: ts=8
