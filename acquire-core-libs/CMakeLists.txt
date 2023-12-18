cmake_minimum_required(VERSION 3.5)
project(acquire-core-libs)
cmake_policy(SET CMP0079 NEW) # use targets from other directories
enable_testing()

include(cmake/aq_require.cmake)
include(cmake/ide.cmake)
include(cmake/msvc.cmake)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)

add_subdirectory(src)
add_subdirectory(tests)

include(CPack)
