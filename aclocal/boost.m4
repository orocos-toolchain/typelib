dnl -*- autoconf -*-
dnl $Id: boost.m4,v 1.8 2005/04/23 10:28:50 sjoyeux Exp $

AC_DEFUN([CLBS_LIB_BOOST],
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

AC_DEFUN([CLBS_BOOST_SUBLIB_DEFINE],
[
    AC_DEFINE(HAVE_BOOST_$2, [], [If the boost/$1 library is available])
    BOOST_$2_CFLAGS="$BOOST_CPPFLAGS"
    BOOST_$2_LDFLAGS="$BOOST_LDFLAGS -lboost_$1"

    AC_SUBST(BOOST_$2_CFLAGS)
    AC_SUBST(BOOST_$2_LDFLAGS)
])

dnl CLBS_BOOST_SUBLIB(libname, header, class, [if found], [if not found])
AC_DEFUN([CLBS_BOOST_SUBLIB],
[
  AC_REQUIRE([CLBS_LIB_BOOST])
  AC_LANG_PUSH(C++)

  clbs_sv_$1_CPPFLAGS="$CPPFLAGS"
  clbs_sv_$1_CFLAGS="$CFLAGS"
  clbs_sv_$1_LDFLAGS="$LDFLAGS"

  CPPFLAGS="$BOOST_CPPFLAGS $CPPFLAGS"
  AC_CHECK_HEADER([$2], [has_working_$1=yes], [has_working_$1=no])
    
  if test "$has_working_$1" = "yes"; then
    AC_MSG_CHECKING([for the Boost/$1 library])
    LDFLAGS="$BOOST_LDFLAGS -lboost_$1 $PTHREAD_LIBS $LDFLAGS"
    AC_LINK_IFELSE(
    [
      #include <$2>

      int main()
      {
        $3 test;
      }
    ], 
    [AC_MSG_RESULT([yes])], 
    [has_working_$1=no
     AC_MSG_RESULT([no])])
  fi

  CPPFLAGS="$clbs_sv_$1_CPPFLAGS"
  CFLAGS="$clbs_sv_$1_CFLAGS"
  LDFLAGS="$clbs_sv_$1_LDFLAGS"
 
  if test "$has_working_$1" = "no"; then
    ifelse([$5], , , $5)
  else
    ifelse([$4], , , $4)
    CLBS_BOOST_SUBLIB_DEFINE($1, translit($1, 'a-z', 'A-Z'))
  fi

  AC_LANG_POP
])

dnl CLBS_BOOST_THREADS([if-found], [if-not-found])
AC_DEFUN([CLBS_BOOST_THREAD],
[
  AC_REQUIRE([CLBS_LIB_BOOST])
  AC_REQUIRE([APR_PTHREADS_CHECK])

  has_working_bthreads=yes
  if test "x$pthreads_working" != "xyes"; then
    AC_MSG_FAILURE([POSIX threads not available])
    has_working_bthreads=no
  fi

  if test "$has_working_bthreads" = "yes"; then
    clbs_sv_CPPFLAGS="$CPPFLAGS"
    clbs_sv_LDFLAGS="$LDFLAGS"
    CPPFLAGS="$PTHREAD_CFLAGS $CPPFLAGS"
    LDFLAGS="$PTHREAD_LIBS $LDFLAGS"
    CLBS_BOOST_SUBLIB(thread, [boost/thread/mutex.hpp], [boost::mutex], [], [has_working_bthreads=no])
    CPPFLAGS=$clbs_sv_CPPFLAGS
    LDFLAGS=$clbs_sv_LDFLAGS
  fi

  if test "$has_working_bthreads" = "no"; then
    ifelse([$2], , , $2)
  else
    ifelse([$1], , , $1)
    BOOST_THREAD_CFLAGS="$BOOST_THREAD_CPPFLAGS $PTHREAD_CFLAGS"
    BOOST_THREAD_LDFLAGS="$BOOST_THREAD_LDFLAGS $PTHREAD_LIBS"
    AC_SUBST(BOOST_THREAD_CFLAGS)
    AC_SUBST(BOOST_THREAD_LDFLAGS)
  fi
])

dnl CLBS_BOOST_REGEX(if-found, if-not-found)
AC_DEFUN([CLBS_BOOST_REGEX], 
[
  CLBS_BOOST_SUBLIB(regex, [boost/regex.hpp], [boost::regex], [$1], [$2])
])

