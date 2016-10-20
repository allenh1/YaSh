# Compile flags
BUILT_SOURCES = shell_bison.hh

if DEBUG
AM_CFLAGS = -g -O0 -lpthread -std=c++14
AM_CXXFLAGS = -g -O0 -lpthread -std=c++14
else
AM_CFLAGS =  -O2 -fPIC -lpthread
AM_CXXFLAGS = -O2 -fPIC -lpthread -std=c++14
endif

AM_YFLAGS = -d
AM_LFLAGS = -o$(LEX_OUTPUT_ROOT).c

CLEAN_FILES = *~

# targets
bin_PROGRAMS = yash
UTILS = shell-utils.hpp shell-utils.cpp
PARSER = wildcard.hpp wildcard.cpp shell_bison.yy shell_flex.ll $(UTILS)
COMMAND = command.cpp command.hpp
READLINE =  shell-readline.cpp shell-readline.hpp
yash_SOURCES = main.cpp $(COMMAND) $(PARSER) $(READLINE)

# don't forget the readline!
EXTRA_DIST = $(READLINE) $(UTILS) $(PARSER) $(COMMAND)
