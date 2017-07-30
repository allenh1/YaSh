#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([YaSh],[0.0.8],[hunter@openrobotics.org])
AC_CONFIG_SRCDIR([src/command.cpp])
AC_CONFIG_MACRO_DIRS([m4])
AC_CONFIG_AUX_DIR(config)

AM_INIT_AUTOMAKE

# Set CXXFLAGS
CXXFLAGS=""
CFLAGS=""

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LN_S
AM_PROG_LEX
AC_PROG_YACC
AC_PROG_LIBTOOL

LT_INIT
AC_CONFIG_HEADER(config.h)

# Check for debug mode
AC_ARG_ENABLE(debug,
AS_HELP_STRING([--enable-debug],
[enable debugging, default: no]),
[case "${enableval}" in
yes) debug=true ;;
no)  debug=false ;;
*)   AC_MSG_ERROR([bad value ${enableval} for --enable-debug]) ;;
esac],
[debug=false])

AM_CONDITIONAL(DEBUG, test x"$debug" = x"true")

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h stdlib.h string.h termios.h unistd.h])
AC_CHECK_HEADERS([alloca.h regex.h sys/ioctl.h sys/types.h sys/wait.h])
AC_CHECK_HEADERS([sys/stat.h dirent.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T

# Checks for library functions.
AC_FUNC_FORK
AC_CHECK_FUNCS([dup2 gethostname memset regcomp setenv strchr strdup strstr])

AC_CONFIG_FILES([Makefile src/Makefile])

# Output
AC_OUTPUT
