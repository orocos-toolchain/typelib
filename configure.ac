dnl configure.in template for common projects

AC_INIT(typeLib, 0.1, sylvain.joyeux@laas.fr)
dnl AC_CONFIG_SRCDIR([add unique file to check source dir])
AC_CONFIG_HEADER([typelib-config.h])

AC_PREREQ(2.54)

############ Language and common setup
dnl AC_LANG(C)
AC_LANG(C++)
AC_CANONICAL_HOST
dnl AC_PROG_CC
AC_PROG_CXX
CLBS_DEBUG_OPTIONS
CLBS_TEST_SUPPORT

############ Check for doxygen availability if Doxygen.in is present
HAVE_DOX="no"
if test -f Doxygen.in; then
    HAVE_DOX="yes" 
    CLBS_INIT_DOXYGEN( , , [
      AC_MSG_NOTICE([Cannot find doxygen])
      HAVE_DOX="no"
    ])
fi
AC_SUBST([HAVE_DOX])

############ Checking for libtool
AC_LIBTOOL_DLOPEN
AC_PROG_LIBTOOL
AC_SUBST(LIBTOOL_DEPS)
AC_PROG_INSTALL

############ Dependencies
CLBS_CHECK_BOOST([], AC_MSG_ERROR([Unable to find the Boost library]))
CLBS_BOOST_FILESYSTEM([], AC_MSG_ERROR([Unable to find the Boost/Filesystem library]))
PKG_CHECK_MODULES(LIBXML, libxml-2.0,, [AC_MSG_ERROR([libxml2 is needed])])
PKG_CHECK_MODULES(UTILMM, utilmm,, [AC_MSG_ERROR([UtilMM is needed])])

############ Output files
AC_CONFIG_FILES(
    typelib.pc
    Init.make
    mk/init.mk
    Makefile
    tools/Makefile
    typelib/Makefile

    lang/Makefile
    lang/cimport/Makefile
    lang/tlb/Makefile

    test_suite/Makefile
    test_suite/testsuite.hh
)

############ Doxygen
dnl is present if Doxyfile.in exists AND doxygen is available
if test "x$HAVE_DOX" = "xyes"; then
   AC_CONFIG_FILES([Doxyfile])
fi

AC_OUTPUT

