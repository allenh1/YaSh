cmake_minimum_required(VERSION 3.8.2)
project(yash VERSION 0.0.8)

# find flex and bison
find_package(BISON)
find_package(FLEX)

# set CXXFLAGS
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -ggdb3 -fsanitize=address -O0")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -O2")

# threads
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# source files
set(sources
  src/shell-utils.cpp
  src/wildcard.cpp
  src/command.cpp
  src/simple_command.cpp
  src/shell-readline.cpp
  src/main.cpp
)

# generated files
bison_target(Parser src/shell_bison.yy ${CMAKE_CURRENT_BINARY_DIR}/shell_bison.cc)
flex_target(Scanner src/shell_flex.ll ${CMAKE_CURRENT_BINARY_DIR}/shell_flex.cc)

add_flex_bison_dependency(Scanner Parser)

include_directories(${CMAKE_CURRENT_BINARY_DIR} src)

add_executable(yash
  ${sources}
  ${BISON_Parser_OUTPUTS}
  ${FLEX_Scanner_OUTPUTS}
)

target_link_libraries(yash Threads::Threads)

set_property(TARGET yash PROPERTY CXX_STANDARD 17)

install(TARGETS yash DESTINATION bin)
