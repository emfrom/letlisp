AC_PREREQ([2.69])
AC_INIT([letlisp], [0.1.1], [vertlaforet@protonmail.com])
AM_INIT_AUTOMAKE([foreign subdir-objects -Wall])
AC_PROG_CC
PKG_PROG_PKG_CONFIG
AC_CONFIG_FILES([Makefile])
PKG_CHECK_MODULES([READLINE], [readline], [],
  [AC_MSG_ERROR([GNU readline not found (missing pkg-config file or development package)])])
PKG_CHECK_MODULES([GC], [bdw-gc])
PKG_CHECK_MODULES([GMP], [gmp >= 6.0])
AC_SUBST([GC_CFLAGS])
AC_SUBST([GC_LIBS])
AC_OUTPUT
