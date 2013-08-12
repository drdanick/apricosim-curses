#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.65])
AC_INIT([aprsim], [0.5], [nick@drdanick.com])
AM_INIT_AUTOMAKE(aprsim, 0.5)
AC_CONFIG_SRCDIR([src/gui.c])
#AC_CONFIG_HEADERS

# Checks for programs.
AC_PROG_CC

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([stdlib.h string.h curses.h])

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.
AC_CHECK_FUNCS([memset])

AC_OUTPUT(Makefile src/Makefile)
