#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.65])
AC_INIT([aprsim], [0.9], [nick@drdanick.com])
AM_INIT_AUTOMAKE(aprsim, 0.9)
AC_CONFIG_SRCDIR([src])
#AC_CONFIG_HEADERS

# Checks for programs.
AC_PROG_CC

# Checks for libraries.
AC_CHECK_LIB(curses, initscr, [], [
        echo "A working curses library is required."
        exit -1
        ])

# Checks for header files.
AC_CHECK_HEADERS([stdlib.h string.h curses.h])

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.
AC_CHECK_FUNCS([memset])

# Feature toggles

# Port emulation
AC_MSG_CHECKING(--enable-port-emulation)
AC_ARG_ENABLE(port-emulation,
                  [  --enable-port-emulation         Enable port emulation.],
                      [enable_ports=$enableval],
                          [enable_ports="no"])
AC_MSG_RESULT($enable_ports)

# Disk emulation
AC_MSG_CHECKING(--enable-disk-emulation)
AC_ARG_ENABLE(disk-emulation,
                  [  --enable-disk-emulation         Enable disk emulation.],
                      [enable_disks=$enableval],
                          [enable_disks="no"])
AC_MSG_RESULT($enable_disks)

# TTY emulation
AC_MSG_CHECKING(--enable-tty-emulation)
AC_ARG_ENABLE(tty-emulation,
                  [  --enable-tty-emulation         Enable terminal emulation.],
                      [enable_tty=$enableval],
                          [enable_tty="no"])
AC_MSG_RESULT($enable_tty)

if test "$enable_ports" = "yes"; then
    AC_DEFINE(PORT_EMU)
fi

if test "$enable_ports" = "yes"; then
    AC_DEFINE(DISK_EMU)
fi

if test "$enable_tty" = "yes"; then
    AC_DEFINE(TTY_EMU)
fi

AC_OUTPUT(Makefile src/Makefile)
