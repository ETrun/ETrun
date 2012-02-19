AC_INIT([ETrun], [1.0], [www.timeruns.net])

AC_CONFIG_AUX_DIR([build-aux])

AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([ETrun], [1.0])

# Init libtool and disable static library creation
LT_INIT(disable-static)

# Compilers we use
AC_PROG_CC

# Auto strip
AC_CHECK_TOOL([STRIP],[strip])

# Check if these headers exist
AC_CONFIG_HEADERS([config.h])

# List of Makefiles we want to generate
AC_CONFIG_FILES([Makefile src/cgame/Makefile src/game/Makefile src/ui/Makefile])

AC_OUTPUT
