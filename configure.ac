#AC_PREREQ([2.63])
AC_INIT([FinalCM], [0.1], [comick@autistici.org])
AM_INIT_AUTOMAKE([
  -Wall
  -Werror
  dist-bzip2
])

# Checks for programs.
AC_PROG_CC
AM_PROG_CC_C_O

# Checks for libraries.
# PKG_CHECK_MODULES(XCB, xcb)
# PKG_CHECK_MODULES(XCB_RENDER, xcb-render)
# PKG_CHECK_MODULES(XCB_RENDERUTIL, xcb-renderutil)
# PKG_CHECK_MODULES(XCB_EVENT, xcb-event)

# Checks for header files.
AC_CHECK_HEADERS([stdint.h stdlib.h string.h])
AC_CONFIG_HEADERS([config.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_INT16_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_FUNC_MALLOC

AC_CONFIG_FILES([
  Makefile
  src/Makefile
])
AC_OUTPUT
