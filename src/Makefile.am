# Compile flags
BUILT_SOURCES = shell_bison.hh

if DEBUG
AM_CFLAGS = -ggdb3 -fsanitize=address -Wall -Wextra -O0 -lpthread -std=c++17
AM_CXXFLAGS = -ggdb3 -O0 -fsanitize=address -Wall -Wextra -lpthread -std=c++17
else
AM_CFLAGS =  -O2 -Wall -Wextra -lpthread
AM_CXXFLAGS = -O2 -Wall -Wextra -lpthread -std=c++17
endif

AM_YFLAGS = -d
AM_LFLAGS = -o$(LEX_OUTPUT_ROOT).c

CLEAN_FILES = *~

# targets
bin_PROGRAMS = yash
UTILS = shell-utils.hpp shell-utils.cpp scope-exit.hpp
PARSER = wildcard.hpp wildcard.cpp shell_bison.yy shell_flex.ll $(UTILS)
SIMPLE_COMMAND = simple_command.cpp simple_command.hpp
COMMAND = command.cpp command.hpp job.hpp
READLINE =  shell-readline.cpp shell-readline.hpp
yash_SOURCES = main.cpp $(COMMAND) $(PARSER) $(READLINE) $(SIMPLE_COMMAND)

# don't forget the readline!
EXTRA_DIST = $(READLINE) $(UTILS) $(PARSER) $(COMMAND) $(SIMPLE_COMMAND)
