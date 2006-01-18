dnl $Rev: 1267 $
dnl $Id: ruby.m4 1267 2006-01-18 15:19:48Z sjoyeux $

dnl MD_CHECK_RUBY(action-if-found, action-if-not-found)
AC_DEFUN([CLBS_CHECK_RUBY], [
    md_ruby_support=yes
    AC_PATH_PROG([RUBY], [ruby])
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

