#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([PyBSP], [1.0], [solker2015@gmail.com])
AM_INIT_AUTOMAKE(PyBSP,1.0)
AC_CONFIG_SRCDIR([src/runtime/BSPArrayPartition.cpp])
AC_CONFIG_SRCDIR([src/frontend/module.cpp])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CXX

# Checks for libraries.
AC_CHECK_LIB(mpi, MPI_Init, [], [echo "Please install MPI first!"; exit -1])
AC_CHECK_LIB(python, Py_Initialize, [], [echo "Please install Python first!"; exit -1])

# Checks for header files.
AC_CHECK_HEADERS([stdint.h])
AC_CHECK_HEADERS([mpi.h])
AC_CHECK_HEADERS([python.h])
AC_CHECK_HEADERS([numpy/arrayobject.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_INT8_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_CONFIG_FILES([Makefile])
AC_OUTPUT


