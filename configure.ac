dnl configure.in template for common projects

AC_INIT(typeLib, 0.3, sylvain.joyeux@laas.fr)
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

############ Checking for libtool
AC_LIBTOOL_DLOPEN
AC_PROG_LIBTOOL
AC_SUBST(LIBTOOL_DEPS)
AC_PROG_INSTALL

############ Dependencies
CLBS_CHECK_DOXYGEN
CLBS_CHECK_ANTLR
CLBS_CHECK_RUBY([], [AC_MSG_WARN([Disabled Ruby support])])
CLBS_CHECK_RDOC([], [AC_MSG_WARN([No documentation for Ruby extension])])
CLBS_CHECK_BOOST([], AC_MSG_ERROR([Unable to find the Boost library]))
CLBS_BOOST_FILESYSTEM([], AC_MSG_ERROR([Unable to find the Boost/Filesystem library]))
PKG_CHECK_MODULES(LIBXML, libxml-2.0,, [AC_MSG_ERROR([libxml2 is needed])])
PKG_CHECK_MODULES(UTILMM, utilmm,, [AC_MSG_ERROR([UtilMM is needed])])
CLBS_TEST_SUPPORT

AC_ARG_ENABLE(programs,
	AC_HELP_STRING([--disable-programs], [do not build the typelib tool]),
	[BUILD_PROGRAMS=$enableval], [BUILD_PROGRAMS=yes])
AC_SUBST(PROGRAMS)

############ Output files
AC_CONFIG_FILES([
    typelib.pc
    Init.make
    mk/init.mk
    Makefile
    tools/Makefile
    typelib/Makefile

    Doxyfile

    lang/Makefile
    lang/cimport/Makefile
    lang/tlb/Makefile

    bindings/Makefile

    test/Makefile
    test/testsuite.hh
    test/ruby/test_config.rb
])

if test "x$HAS_RUBY_SUPPORT" = "xyes"; then
AC_CONFIG_FILES([ 
    bindings/ruby/Makefile
    bindings/ruby/typelib_ruby.pc
    bindings/ruby/ext/Makefile
    test/ruby/Makefile
    ])
fi
    
AC_OUTPUT

