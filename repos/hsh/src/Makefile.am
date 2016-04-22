# Compile flags
BUILT_SOURCES = shell_bison.hh
AM_CFLAGS = -O1 -fPIC
AM_CXXFLAGS = -O1 -fPIC -std=c++14

AM_YFLAGS = -d
AM_LFLAGS = -o$(LEX_OUTPUT_ROOT).c

CLEAN_FILES = *~

# targets
bin_PROGRAMS = hsh
PARSER = shell_bison.yy shell_flex.ll
READLINE = shell-readline.hpp
hsh_SOURCES = command.cpp command.hpp $(PARSER)

# don't forget the readline!
EXTRA_DIST = $(READLINE)
