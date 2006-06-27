dnl $Rev: 1494 $
dnl $Id: ruby.m4 1494 2006-06-27 07:53:44Z sjoyeux $

dnl CLBS_CHECK_RUBY(action-if-found, action-if-not-found)
dnl defines: RUBY, RUBY_EXTENSION_BASEDIR, RUBY_VERSION, HAS_RUBY_SUPPORT
AC_DEFUN([CLBS_CHECK_RUBY], [
    md_ruby_support=yes
    AC_ARG_VAR(RUBY, [the Ruby interpreter])
    if test -z "$RUBY"; then
        AC_PATH_PROGS([RUBY], [ruby ruby1.8])
    fi
    test -z "$RUBY" && md_ruby_support=no

    if test "$md_ruby_support" = "yes"; then
        [RUBY_EXTENSION_BASEDIR=`$RUBY -rrbconfig -e "puts Config::MAKEFILE_CONFIG['topdir']"`]
        [RUBY_VERSION=`$RUBY -rrbconfig -e "puts Config::CONFIG['ruby_version']"`]
        
        md_ruby_cppflags=$CPPFLAGS
        CPPFLAGS="$CPPFLAGS -I$RUBY_EXTENSION_BASEDIR"
        AC_CHECK_HEADER([ruby.h], [], [md_ruby_support=no])
    fi

    AS_IF([test "$md_ruby_support" = "yes"], [
        HAS_RUBY_SUPPORT=yes
        AC_SUBST(RUBY_EXTENSION_BASEDIR)
        AC_SUBST(RUBY_VERSION)
        AC_SUBST(HAS_RUBY_SUPPORT)
        ifelse([$1], [], [], [$1])
    ], [
        AC_MSG_WARN([Ruby support disabled])
        HAS_RUBY_SUPPORT=no
        AC_SUBST(HAS_RUBY_SUPPORT)
        ifelse([$2], [], [], [$2])
    ])
])

dnl CLBS_CHECK_RDOC(action-if-found, action-if-not-found)
dnl defines: RDOC, HAS_RDOC
AC_DEFUN([CLBS_CHECK_RDOC], [
    md_rdoc_support=yes
    AC_ARG_VAR(RDOC, [the RDoc utility])
    if test -z "$RDOC"; then
        AC_PATH_PROG([RDOC], [rdoc rdoc1.8])
    fi
    test -z "$RDOC" && md_rdoc_support=no

    AS_IF([test "$md_rdoc_support" = "yes"], [
        HAS_RDOC=yes
        AC_SUBST(HAS_RDOC)
        ifelse([$1], [], [], [$1])
    ], [
        AC_MSG_WARN([RDoc support disabled])
        HAS_RDOC=no
        AC_SUBST(HAS_RDOC)
        ifelse([$2], [], [], [$2])
    ])
])

