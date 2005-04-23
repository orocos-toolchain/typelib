dnl -*- autoconf -*-
dnl $Id: boost.m4,v 1.6 2005/01/10 12:08:45 fpy Exp $

AC_DEFUN([AC_CHECK_BOOST],
 [
  AC_SUBST(BOOST_CPPFLAGS)
  AC_SUBST(BOOST_LDFLAGS)

  AC_DEFINE([HAVE_BOOST], [], [If the Boost libraries are available])
  
  BOOST_CPPFLAGS=""
  BOOST_LDFLAGS=""

  BOOST_ROOT=""

dnl Extract the path name from a --with-boost=PATH argument
  AC_ARG_WITH(boost,
	AC_HELP_STRING([--with-boost=PATH],
	               [absolute path where the Boost C++ libraries reside]),
	[
	 if test "x$withval" = "x" ; then
	   BOOST_ROOT=""
	 else
	   BOOST_ROOT="$withval"
	   BOOST_TMP_CPPFLAGS="-I$BOOST_ROOT/include"
	   BOOST_TMP_LDFLAGS="-L$BOOST_ROOT/lib"
    	 fi
	])

dnl Checking for Boost headers
   CPPFLAGS_OLD=$CPPFLAGS
   CPPFLAGS="$CPPFLAGS $BOOST_TMP_CPPFLAGS"
   AC_LANG_SAVE
   AC_LANG_CPLUSPLUS
   AC_CHECK_HEADER([boost/version.hpp], [have_boost="yes"])
   AC_LANG_RESTORE
   CPPFLAGS=$CPPFLAGS_OLD 
   if test "x$have_boost" = "xyes" ; then 
     ifelse([$1], , :, [$1])
     BOOST_CPPFLAGS="$BOOST_TMP_CPPFLAGS"
     BOOST_LDFLAGS=$BOOST_TMP_LDFLAGS
     AC_DEFINE(HAVE_BOOST)
   else
     ifelse([$2], , :, [$2])
   fi
 ]
)
