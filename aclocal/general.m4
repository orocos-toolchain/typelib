dnl -*- autoconf -*-
dnl $Id: general.m4,v 1.1 2005/02/11 14:51:00 fpy Exp $

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