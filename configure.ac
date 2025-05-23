AC_PREREQ([2.69])
AC_INIT([minilisp], [0.0.2j], [vertlaforet@protonmail.com])
AM_INIT_AUTOMAKE([foreign subdir-objects -Wall])
AC_PROG_CC
PKG_PROG_PKG_CONFIG
AC_CONFIG_FILES([Makefile])
PKG_CHECK_MODULES([READLINE], [readline], [],
  [AC_MSG_ERROR([GNU readline not found (missing pkg-config file or development package)])])
AC_OUTPUT
