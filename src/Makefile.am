# Compile flags
BUILT_SOURCES = shell_bison.hh
AM_CFLAGS = -O2 -fPIC -lpthread
AM_CXXFLAGS = -O2 -fPIC -lpthread -std=c++14

AM_YFLAGS = -d
AM_LFLAGS = -o$(LEX_OUTPUT_ROOT).c

CLEAN_FILES = *~

# targets
bin_PROGRAMS = yash
PARSER = wildcard.hpp wildcard.cpp shell_bison.yy shell_flex.ll 
READLINE = shell-readline.hpp
yash_SOURCES = command.cpp command.hpp $(PARSER)

# don't forget the readline!
EXTRA_DIST = $(READLINE)
